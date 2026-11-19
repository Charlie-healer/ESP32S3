import Foundation
import CoreLocation

/// Feeds the round screen's compass page while it is visible:
///   #C,<heading>,<kmh>
/// Heading comes from Core Location (magnetic/true), speed from GPS.
final class CompassFeeder: NSObject, CLLocationManagerDelegate {
    static let shared = CompassFeeder()

    private let manager = CLLocationManager()
    private var running = false
    private var heading = -1.0
    private var speedMps = 0.0
    private var lastSend = 0.0

    private override init() {
        super.init()
        manager.delegate = self
        manager.desiredAccuracy = kCLLocationAccuracyBest
        manager.headingFilter = 1
        manager.distanceFilter = 1
    }

    func start() {
        if running { return }
        running = true
        if manager.authorizationStatus == .notDetermined {
            manager.requestWhenInUseAuthorization()
        }
        if CLLocationManager.headingAvailable() {
            manager.startUpdatingHeading()
        }
        manager.startUpdatingLocation()
        print("CompassFeeder start")
    }

    func stop() {
        if !running { return }
        running = false
        manager.stopUpdatingHeading()
        manager.stopUpdatingLocation()
        heading = -1
        speedMps = 0
        print("CompassFeeder stop")
    }

    func locationManager(_ manager: CLLocationManager, didUpdateHeading newHeading: CLHeading) {
        let h = newHeading.trueHeading >= 0 ? newHeading.trueHeading : newHeading.magneticHeading
        if h >= 0 && h < 360 { heading = h }
        send()
    }

    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        if let l = locations.last, l.speed >= 0 {
            speedMps = max(0, l.speed)
            if heading < 0, l.course >= 0 { heading = l.course }
        }
        send()
    }

    func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {
        print("CompassFeeder location error: \(error.localizedDescription)")
    }

    private func send() {
        guard running else { return }
        let now = Date().timeIntervalSince1970
        if now - lastSend < 0.4 { return }
        lastSend = now
        let h = heading >= 0 ? Int(heading.rounded()) % 360 : -1
        let kmh = min(300, max(0, speedMps * 3.6))
        BleLink.shared.send("#C,\(h),\(String(format: "%.1f", kmh))")
    }
}
