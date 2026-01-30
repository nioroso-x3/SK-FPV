#!/usr/bin/env python3
"""
Fake MAVLink Vehicle

Simulates a basic vehicle that:
1. Sends heartbeats so QGroundControl sees it
2. Sends basic telemetry (position, attitude, battery, etc.)
3. Receives and displays RC_CHANNELS_OVERRIDE messages

Usage:
    python fake_vehicle.py --port 14550
    
Then connect QGroundControl to UDP localhost:14550
"""

import time
import math
import argparse
import threading
from pymavlink import mavutil
from pymavlink.dialects.v20 import common as mavlink

class FakeVehicle:
    
    # ArduPilot mode mappings per vehicle type
    COPTER_MODES = {
        0: 'STABILIZE',
        1: 'ACRO',
        2: 'ALT_HOLD',
        3: 'AUTO',
        4: 'GUIDED',
        5: 'LOITER',
        6: 'RTL',
        7: 'CIRCLE',
        8: 'POSITION',
        9: 'LAND',
        10: 'OF_LOITER',
        11: 'DRIFT',
        13: 'SPORT',
        14: 'FLIP',
        15: 'AUTOTUNE',
        16: 'POSHOLD',
        17: 'BRAKE',
        18: 'THROW',
        19: 'AVOID_ADSB',
        20: 'GUIDED_NOGPS',
        21: 'SMART_RTL',
        22: 'FLOWHOLD',
        23: 'FOLLOW',
        24: 'ZIGZAG',
        25: 'SYSTEMID',
        26: 'AUTOROTATE',
        27: 'AUTO_RTL',
    }
    
    PLANE_MODES = {
        0: 'MANUAL',
        1: 'CIRCLE',
        2: 'STABILIZE',
        3: 'TRAINING',
        4: 'ACRO',
        5: 'FLY_BY_WIRE_A',
        6: 'FLY_BY_WIRE_B',
        7: 'CRUISE',
        8: 'AUTOTUNE',
        10: 'AUTO',
        11: 'RTL',
        12: 'LOITER',
        13: 'TAKEOFF',
        14: 'AVOID_ADSB',
        15: 'GUIDED',
        17: 'QSTABILIZE',
        18: 'QHOVER',
        19: 'QLOITER',
        20: 'QLAND',
        21: 'QRTL',
        22: 'QAUTOTUNE',
        23: 'QACRO',
        24: 'THERMAL',
        25: 'LOITER_ALT_QLAND',
    }
    
    ROVER_MODES = {
        0: 'MANUAL',
        1: 'ACRO',
        3: 'STEERING',
        4: 'HOLD',
        5: 'LOITER',
        6: 'FOLLOW',
        7: 'SIMPLE',
        10: 'AUTO',
        11: 'RTL',
        12: 'SMART_RTL',
        15: 'GUIDED',
    }
    
    SUB_MODES = {
        0: 'STABILIZE',
        1: 'ACRO',
        2: 'ALT_HOLD',
        3: 'AUTO',
        4: 'GUIDED',
        7: 'CIRCLE',
        9: 'SURFACE',
        16: 'POSHOLD',
        19: 'MANUAL',
    }
    
    VEHICLE_TYPES = {
        'copter': (mavlink.MAV_TYPE_QUADROTOR, COPTER_MODES),
        'plane': (mavlink.MAV_TYPE_FIXED_WING, PLANE_MODES),
        'rover': (mavlink.MAV_TYPE_GROUND_ROVER, ROVER_MODES),
        'sub': (mavlink.MAV_TYPE_SUBMARINE, SUB_MODES),
    }
    
    def __init__(self, connection_string: str, system_id: int = 1, component_id: int = 1, vehicle_type: str = 'plane'):
        self.system_id = system_id
        self.component_id = component_id
        
        # Vehicle type
        vehicle_type = vehicle_type.lower()
        if vehicle_type not in self.VEHICLE_TYPES:
            raise ValueError(f"Unknown vehicle type: {vehicle_type}. Choose from: {list(self.VEHICLE_TYPES.keys())}")
        
        self.vehicle_type = vehicle_type
        self.mav_type, self.MODE_NAMES = self.VEHICLE_TYPES[vehicle_type]
        
        print(f"Vehicle type: {vehicle_type.upper()} (MAV_TYPE={self.mav_type})")
        
        # Create connection
        print(f"Creating MAVLink connection: {connection_string}")
        self.conn = mavutil.mavlink_connection(
            connection_string,
            source_system=system_id,
            source_component=component_id
        )
        
        # Simulated vehicle state
        self.lat = 37.7749 * 1e7    # San Francisco (degE7)
        self.lon = -122.4194 * 1e7
        self.alt_msl = 100000       # 100m MSL (mm)
        self.alt_agl = 50000        # 50m AGL (mm)
        self.heading = 0            # centidegrees
        self.roll = 0.0             # radians
        self.pitch = 0.0
        self.yaw = 0.0
        self.groundspeed = 5.0      # m/s
        self.airspeed = 6.0
        self.battery_voltage = 12600  # mV
        self.battery_remaining = 75   # percent
        
        # Vehicle mode state
        self.armed = False
        self.custom_mode = 0        # ArduPilot custom mode
        self.base_mode = mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED
        
        # RC override tracking
        self.last_rc_override = None
        self.rc_override_count = 0
        
        self._running = False
        
    def start(self):
        """Start the fake vehicle"""
        self._running = True
        
        # Start receiver thread
        self._recv_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self._recv_thread.start()
        
        # Start telemetry thread
        self._telem_thread = threading.Thread(target=self._telemetry_loop, daemon=True)
        self._telem_thread.start()
        
        print(f"Fake vehicle started (sysid={self.system_id})")
        print("Waiting for GCS connection...")
        
    def stop(self):
        """Stop the fake vehicle"""
        self._running = False
        self.conn.close()
        
    def _receive_loop(self):
        """Receive and handle incoming messages"""
        while self._running:
            try:
                msg = self.conn.recv_match(blocking=True, timeout=0.1)
                if msg is None:
                    continue
                
                msg_type = msg.get_type()
                
                # Handle RC_CHANNELS_OVERRIDE
                if msg_type == 'RC_CHANNELS_OVERRIDE':
                    self.rc_override_count += 1
                    self.last_rc_override = msg
                    
                    # Print RC override details
                    channels = [
                        msg.chan1_raw, msg.chan2_raw, msg.chan3_raw, msg.chan4_raw,
                        msg.chan5_raw, msg.chan6_raw, msg.chan7_raw, msg.chan8_raw
                    ]
                    
                    # Filter out unused channels (0 or 65535)
                    active = [(i+1, ch) for i, ch in enumerate(channels) if 0 < ch < 65535]
                    
                    if active:
                        ch_str = ", ".join([f"CH{i}={v}" for i, v in active])
                        print(f"\r[RC_OVERRIDE #{self.rc_override_count}] {ch_str}          ", end='', flush=True)
                
                # Handle COMMAND_LONG (arm/disarm, etc.)
                elif msg_type == 'COMMAND_LONG':
                    self._handle_command(msg)
                
                # Handle HEARTBEAT from GCS
                elif msg_type == 'HEARTBEAT':
                    if msg.type == mavlink.MAV_TYPE_GCS:
                        pass  # GCS connected
                
                # Handle MANUAL_CONTROL
                elif msg_type == 'MANUAL_CONTROL':
                    print(f"\r[MANUAL_CONTROL] x={msg.x}, y={msg.y}, z={msg.z}, r={msg.r}          ", end='', flush=True)
                
                # Handle SET_MODE (older method)
                elif msg_type == 'SET_MODE':
                    old_mode_name = self.MODE_NAMES.get(self.custom_mode, f"UNKNOWN({self.custom_mode})")
                    new_custom_mode = msg.custom_mode
                    new_mode_name = self.MODE_NAMES.get(new_custom_mode, f"UNKNOWN({new_custom_mode})")
                    
                    self.custom_mode = new_custom_mode
                    self.base_mode = msg.base_mode
                    if self.armed:
                        self.base_mode |= mavlink.MAV_MODE_FLAG_SAFETY_ARMED
                    
                    print(f"\n[SET_MODE] {old_mode_name} -> {new_mode_name} (custom_mode={new_custom_mode})")
                        
            except Exception as e:
                if self._running:
                    print(f"Receive error: {e}")
    
    def _handle_command(self, msg):
        """Handle COMMAND_LONG messages"""
        cmd = msg.command
        
        if cmd == mavlink.MAV_CMD_COMPONENT_ARM_DISARM:
            arm = msg.param1 == 1
            self.armed = arm
            if arm:
                self.base_mode |= mavlink.MAV_MODE_FLAG_SAFETY_ARMED
            else:
                self.base_mode &= ~mavlink.MAV_MODE_FLAG_SAFETY_ARMED
            print(f"\n[COMMAND] {'ARM' if arm else 'DISARM'} - {'ACCEPTED' if True else 'REJECTED'}")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        elif cmd == mavlink.MAV_CMD_DO_SET_MODE:
            # param1 = base mode, param2 = custom mode
            new_base_mode = int(msg.param1)
            new_custom_mode = int(msg.param2)
            
            old_mode_name = self.MODE_NAMES.get(self.custom_mode, f"UNKNOWN({self.custom_mode})")
            new_mode_name = self.MODE_NAMES.get(new_custom_mode, f"UNKNOWN({new_custom_mode})")
            
            self.custom_mode = new_custom_mode
            # Preserve armed state when changing mode
            if self.armed:
                self.base_mode = new_base_mode | mavlink.MAV_MODE_FLAG_SAFETY_ARMED
            else:
                self.base_mode = new_base_mode & ~mavlink.MAV_MODE_FLAG_SAFETY_ARMED
            
            print(f"\n[MODE CHANGE] {old_mode_name} -> {new_mode_name} (custom_mode={new_custom_mode})")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        elif cmd == mavlink.MAV_CMD_REQUEST_MESSAGE:
            msg_id = int(msg.param1)
            print(f"\n[COMMAND] Request message {msg_id}")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        elif cmd == mavlink.MAV_CMD_NAV_TAKEOFF:
            print(f"\n[COMMAND] TAKEOFF to {msg.param7}m")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        elif cmd == mavlink.MAV_CMD_NAV_LAND:
            print(f"\n[COMMAND] LAND")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        elif cmd == mavlink.MAV_CMD_NAV_RETURN_TO_LAUNCH:
            print(f"\n[COMMAND] RTL")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
            
        else:
            print(f"\n[COMMAND] cmd={cmd} p1={msg.param1} p2={msg.param2} p3={msg.param3}")
            self.conn.mav.command_ack_send(cmd, mavlink.MAV_RESULT_ACCEPTED)
    
    def _telemetry_loop(self):
        """Send telemetry at regular intervals"""
        last_heartbeat = 0
        last_position = 0
        last_attitude = 0
        last_battery = 0
        last_status = 0
        
        while self._running:
            now = time.time()
            
            # Heartbeat at 1 Hz
            if now - last_heartbeat >= 1.0:
                self._send_heartbeat()
                last_heartbeat = now
            
            # Position at 5 Hz
            if now - last_position >= 0.2:
                self._send_position()
                last_position = now
            
            # Attitude at 10 Hz
            if now - last_attitude >= 0.1:
                self._send_attitude()
                last_attitude = now
            
            # Battery at 1 Hz
            if now - last_battery >= 1.0:
                self._send_battery()
                last_battery = now
            
            # System status at 1 Hz
            if now - last_status >= 1.0:
                self._send_sys_status()
                last_status = now
            
            # Simulate some movement
            self._update_state()
            
            time.sleep(0.02)  # 50 Hz loop
    
    def _send_heartbeat(self):
        """Send heartbeat message"""
        self.conn.mav.heartbeat_send(
            self.mav_type,
            mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA,
            self.base_mode,
            self.custom_mode,
            mavlink.MAV_STATE_ACTIVE
        )
    
    def _send_position(self):
        """Send global position"""
        self.conn.mav.global_position_int_send(
            int(time.time() * 1000) % (2**32),  # time_boot_ms
            int(self.lat),
            int(self.lon),
            int(self.alt_msl),
            int(self.alt_agl),
            int(self.groundspeed * 100),  # vx cm/s
            0,  # vy
            0,  # vz
            int(self.heading)
        )
        
        # Also send GPS_RAW_INT
        self.conn.mav.gps_raw_int_send(
            int(time.time() * 1000000) % (2**64),  # time_usec
            3,  # fix_type: 3D fix
            int(self.lat),
            int(self.lon),
            int(self.alt_msl),
            65535,  # eph (unknown)
            65535,  # epv (unknown)
            int(self.groundspeed * 100),  # vel cm/s
            int(self.heading),  # cog
            12  # satellites visible
        )
    
    def _send_attitude(self):
        """Send attitude"""
        self.conn.mav.attitude_send(
            int(time.time() * 1000) % (2**32),
            self.roll,
            self.pitch,
            self.yaw,
            0.0,  # rollspeed
            0.0,  # pitchspeed
            0.0   # yawspeed
        )
    
    def _send_battery(self):
        """Send battery status"""
        # Use SYS_STATUS for battery info instead - more compatible
        pass  # Battery info is sent in _send_sys_status
    
    def _send_sys_status(self):
        """Send system status with battery info"""
        self.conn.mav.sys_status_send(
            0b11111111,  # onboard_control_sensors_present
            0b11111111,  # onboard_control_sensors_enabled
            0b11111111,  # onboard_control_sensors_health
            500,  # load (50%)
            int(self.battery_voltage),  # voltage_battery (mV)
            -1,   # current_battery
            int(self.battery_remaining),  # battery_remaining (%)
            0, 0, 0, 0, 0, 0  # drop rates, errors
        )
    
    def _update_state(self):
        """Simulate slow movement/drift"""
        # Slow circular movement
        t = time.time() * 0.1
        radius = 0.0001  # ~10m radius in degrees
        
        base_lat = 37.7749
        base_lon = -122.4194
        
        self.lat = (base_lat + radius * math.sin(t)) * 1e7
        self.lon = (base_lon + radius * math.cos(t)) * 1e7
        
        # Heading follows movement
        self.heading = int((math.degrees(t) % 360) * 100)
        self.yaw = math.radians(self.heading / 100)
        
        # Small attitude variations
        self.roll = 0.05 * math.sin(t * 2)
        self.pitch = 0.03 * math.cos(t * 1.5)
        
        # Battery slowly drains
        self.battery_remaining = max(0, self.battery_remaining - 0.0001)
        self.battery_voltage = int(11000 + self.battery_remaining * 20)
    
    def run(self):
        """Run until interrupted"""
        self.start()
        
        try:
            print("\nMonitoring for RC_CHANNELS_OVERRIDE messages...")
            print("Press Ctrl+C to stop\n")
            
            while True:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n\nStopping...")
        finally:
            self.stop()
            print(f"Total RC_OVERRIDE messages received: {self.rc_override_count}")


def main():
    parser = argparse.ArgumentParser(description='Fake MAVLink Vehicle')
    parser.add_argument(
        '--port', '-p',
        type=int,
        default=14550,
        help='UDP port to listen on (default: 14550)'
    )
    parser.add_argument(
        '--connection', '-c',
        default=None,
        help='MAVLink connection string (overrides --port)'
    )
    parser.add_argument(
        '--sysid', '-s',
        type=int,
        default=1,
        help='System ID (default: 1)'
    )
    parser.add_argument(
        '--vehicle', '-v',
        choices=['copter', 'plane', 'rover', 'sub'],
        default='plane',
        help='Vehicle type (default: plane)'
    )
    
    args = parser.parse_args()
    
    if args.connection:
        conn_string = args.connection
    else:
        # UDP listen mode - QGC connects to us
        conn_string = f"udpin:0.0.0.0:{args.port}"
    
    print(f"Fake MAVLink Vehicle")
    print(f"====================")
    print(f"Connection: {conn_string}")
    print(f"System ID: {args.sysid}")
    print()
    
    vehicle = FakeVehicle(conn_string, system_id=args.sysid, vehicle_type=args.vehicle)
    vehicle.run()


if __name__ == '__main__':
    main()
