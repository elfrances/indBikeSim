/*
    indBikeSim - An app that simulates a basic FTMS indoor bike

    Copyright (C) 2025  Marcelo Mourier  marcelo_mourier@yahoo.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

// By convention the name of the configuration options is "CONFIG_XXXX"
// Defining (undefining) the option includes (excludes) the feature.

// BLE Controller: allows the app to discover an connect to BLE sensor
// devices, such as HR monitors or pedal power sensors.
#undef CONFIG_BLE_CONTROLLER

// Command Line Interface: adds an interactive CLI helpful for debugging
#define CONFIG_CLI

// FIT activity file: allows the app to use the metrics from a FIT file
// to generate the cadence, heart rate, and power values.
#define CONFIG_FIT_ACTIVITY_FILE

// mDNS Agent: allows the app to advertise and discover services via mDNS
#define CONFIG_MDNS_AGENT

// MsgLog: enables the trace message logging facility
#define CONFIG_MSGLOG

