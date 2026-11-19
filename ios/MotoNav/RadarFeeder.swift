import Foundation

// MARK: - ADS-B radar feeder
//
// The round display has no network and no absolute lat/lon, so while its RADAR
// page is open it asks this feeder for aircraft relative to the bike. Same
// 20-byte-per-aircraft bulk type 5 frame as the Android app; the device only
// parses and draws.
//
// Sources: adsb.fi (attribution required, personal non-commercial) and
// adsb.lol (descriptive User-Agent). No API key.
final class RadarFeeder {
    static let shared = RadarFeeder()

    private struct P {
        let dx: Int, dy: Int, alt: Int, trk: Int, gs: Int, sq: Int, flags: Int
        let cs: String
        let dist: Double
    }

    private var active = false
    private var rangeM = 20000
    private var lat = Double.nan
    private var lon = Double.nan
    private var seq = 0
    private var timer: DispatchSourceTimer?
    private let queue = DispatchQueue(label: "motonav.radar")
    private var inFlight = false
    private var failUntil = Date.distantPast

    private init() {}

    func onLocation(lat: Double, lng: Double) {
        self.lat = lat
        self.lon = lng
    }

    func setActive(_ on: Bool) {
        if on == active { return }
        active = on
        if on { start() } else { stop() }
    }

    func setRange(_ meters: Int) {
        if (5000...60000).contains(meters) { rangeM = meters }
    }

    private func start() {
        stop()
        failUntil = .distantPast
        let t = DispatchSource.makeTimerSource(queue: queue)
        t.schedule(deadline: .now() + 0.2, repeating: 2.0)
        t.setEventHandler { [weak self] in self?.tick() }
        t.resume()
        timer = t
    }

    private func stop() {
        timer?.cancel()
        timer = nil
    }

    private func tick() {
        guard active, !inFlight, lat.isFinite, lon.isFinite, Date() >= failUntil else { return }
        inFlight = true
        fetch(source: 0) { [weak self] ok in
            guard let self else { return }
            if ok {
                self.inFlight = false
                return
            }
            self.fetch(source: 1) { ok2 in
                self.inFlight = false
                if !ok2 { self.failUntil = Date().addingTimeInterval(15) }
            }
        }
    }

    private func fetch(source: Int, done: @escaping (Bool) -> Void) {
        let nm = max(1, Int((Double(rangeM) / 1000.0 * 0.539957).rounded()))
        let posix = Locale(identifier: "en_US_POSIX")
        let urlString = source == 0
            ? String(format: "https://opendata.adsb.fi/api/v3/lat/%.5f/lon/%.5f/dist/%d",
                     locale: posix, arguments: [lat, lon, nm])
            : String(format: "https://api.adsb.lol/v2/point/%.5f/%.5f/%d",
                     locale: posix, arguments: [lat, lon, nm])
        guard let url = URL(string: urlString) else { done(false); return }
        var req = URLRequest(url: url)
        req.timeoutInterval = 8
        req.setValue("MotoNav/0.16.0 (iOS; personal non-commercial)", forHTTPHeaderField: "User-Agent")
        req.setValue("application/json", forHTTPHeaderField: "Accept")
        URLSession.shared.dataTask(with: req) { [weak self] data, resp, _ in
            guard let self,
                  let data,
                  (resp as? HTTPURLResponse)?.statusCode == 200,
                  let planes = self.parse(data) else {
                done(false)
                return
            }
            let payload = self.encode(planes, source: source)
            var id = 0
            self.queue.sync {
                id = self.seq & 0xFF
                self.seq += 1
            }
            DispatchQueue.main.async {
                BleLink.shared.sendBulk(type: 5, id: id, data: payload)
            }
            done(true)
        }.resume()
    }

    private func parse(_ data: Data) -> [P]? {
        guard let obj = try? JSONSerialization.jsonObject(with: data),
              let root = obj as? [String: Any] else { return nil }
        let arr = (root["ac"] as? [[String: Any]]) ?? (root["aircraft"] as? [[String: Any]]) ?? []
        let cosLat = cos(lat * .pi / 180)
        var out: [P] = []
        for o in arr {
            guard let ala = (o["lat"] as? NSNumber)?.doubleValue,
                  let alo = (o["lon"] as? NSNumber)?.doubleValue else { continue }
            let dx = Int(((alo - lon) * 111320.0 * cosLat).rounded())
            let dy = Int(((ala - lat) * 110540.0).rounded())
            if abs(dx) > 32000 || abs(dy) > 32000 { continue }
            let dist = Double(dx * dx + dy * dy).squareRoot()
            if dist > Double(rangeM) { continue }

            let altRaw = o["alt_baro"]
            let ground = (altRaw as? String)?.lowercased() == "ground"
            let alt: Int
            if ground {
                alt = 0
            } else if let n = altRaw as? NSNumber {
                alt = max(0, min(60000, n.intValue))
            } else {
                alt = 0x7FFF
            }
            let trkRaw = (o["track"] as? NSNumber)?.doubleValue
                ?? (o["true_heading"] as? NSNumber)?.doubleValue
            let trk = trkRaw.map { max(0, min(35999, Int(($0 * 100).rounded()))) } ?? 0xFFFF
            let gs = (o["gs"] as? NSNumber).map {
                max(0, min(65534, Int(($0.doubleValue * 10).rounded())))
            } ?? 0xFFFF
            let squawk = (o["squawk"] as? String).flatMap(Int.init)
                ?? (o["squawk"] as? NSNumber)?.intValue ?? -1
            let sq = squawk >= 0 ? squawk : 0xFFFF
            var cs = (o["flight"] as? String ?? "")
                .trimmingCharacters(in: .whitespaces).uppercased()
            if cs.count > 6 { cs = String(cs.prefix(6)) }
            while cs.count < 6 { cs += " " }

            var flags = 0
            if ground { flags |= 1 }
            if [7500, 7600, 7700].contains(squawk) { flags |= 2 }
            if let db = (o["dbFlags"] as? NSNumber)?.intValue, (db & 1) != 0 { flags |= 4 }
            out.append(P(dx: dx, dy: dy, alt: alt, trk: trk, gs: gs, sq: sq,
                         flags: flags, cs: cs, dist: dist))
        }
        out.sort { $0.dist < $1.dist }
        return Array(out.prefix(30))
    }

    private func encode(_ planes: [P], source: Int) -> Data {
        var b: [UInt8] = [1, UInt8(planes.count), UInt8(source), 0]
        b += le16(rangeM)
        for p in planes {
            b += le16(p.dx)
            b += le16(p.dy)
            b += le16(p.alt)
            b += le16(p.trk)
            b += le16(p.gs)
            b += le16(p.sq)
            b.append(UInt8(p.flags & 0xFF))
            b.append(0)
            let cs = Array(p.cs.utf8)
            for k in 0..<6 { b.append(k < cs.count ? cs[k] : 32) }
        }
        return Data(b)
    }

    private func le16(_ value: Int) -> [UInt8] {
        [UInt8(value & 0xFF), UInt8((value >> 8) & 0xFF)]
    }
}
