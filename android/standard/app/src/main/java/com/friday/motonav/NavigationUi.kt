package com.friday.motonav

import android.content.Context

object NavigationUi {
    fun active(): Boolean = (WebMotorGuide.active && !WebMotorGuide.arrived) || NaviBridge.hasActiveNavigation()
}

