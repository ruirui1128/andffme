package com.mind.andffme

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob


fun IoScope() = CoroutineScope(SupervisorJob() + Dispatchers.IO)