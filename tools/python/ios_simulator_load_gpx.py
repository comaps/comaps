#!/usr/bin/env python3
"""
GPX/KML/KMZ to iOS Simulator or physical-device location command

Replays a GPX, KML or KMZ track through simctl or Xcode's devicectl for
realistic iOS location simulation.

Tested with CoMaps exported tracks

Usage:
    python ios_simulator_load_gpx.py test_route.gpx
    python ios_simulator_load_gpx.py test_route.kmz
    python ios_simulator_load_gpx.py test_route.gpx --physical-device DEVICE_UDID

Returning a physical device to its real GPS location:
    xcrun devicectl device simulate location clear --device DEVICE_UDID
If you forget this you can always restart the device to restore the normal location.
"""

import argparse
import json
import shlex
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path


def extract_track_points_from_gpx(gpx_file: Path):
    """Extract track points from GPX file."""
    tree = ET.parse(gpx_file)
    root = tree.getroot()
    
    points = []
    # Find all elements with lat/lon attributes
    for elem in root.findall('.//*[@lat][@lon]'):
        lat = float(elem.get('lat'))
        lon = float(elem.get('lon'))
        points.append((lat, lon))
    
    return points

def local_name(tag):
    return tag.rsplit('}', 1)[-1]

def parse_kml_coordinates(text):
    points = []
    for tuple_str in text.split():
        parts = tuple_str.split(',')
        if len(parts) < 2:
            continue
        points.append((float(parts[1]), float(parts[0])))
    
    return points

def parse_gx_coord(text):
    parts = text.split()
    if len(parts) < 2:
        return None
    
    return (float(parts[1]), float(parts[0]))

def extract_track_points_from_kml(kml_data):
    root = ET.fromstring(kml_data)
    
    track_points = []
    placemark_points = []
    for elem in root.iter():
        tag = local_name(elem.tag)
        if tag in ('LineString', 'LinearRing', 'Track'):
            for child in elem.iter():
                child_tag = local_name(child.tag)
                if child_tag == 'coordinates' and child.text:
                    track_points.extend(parse_kml_coordinates(child.text))
                elif child_tag == 'coord' and child.text:
                    point = parse_gx_coord(child.text)
                    if point:
                        track_points.append(point)
        elif tag == 'Point':
            for child in elem.iter():
                if local_name(child.tag) == 'coordinates' and child.text:
                    placemark_points.extend(parse_kml_coordinates(child.text))
    
    return track_points if len(track_points) >= 2 else placemark_points

def extract_track_points_from_kmz(kmz_file: Path):
    with zipfile.ZipFile(kmz_file) as archive:
        names = [n for n in archive.namelist() if n.lower().endswith('.kml')]
        if not names:
            raise ValueError(f"No .kml entry found inside '{kmz_file}'")
        
        name = next((n for n in names if n.lower().endswith('doc.kml')), names[0])
        
        return extract_track_points_from_kml(archive.read(name))

def extract_track_points(track_file: Path):
    suffix = track_file.suffix.lower()
    if suffix == '.kmz':
        return extract_track_points_from_kmz(track_file)
    if suffix == '.kml':
        return extract_track_points_from_kml(track_file.read_bytes())
    
    return extract_track_points_from_gpx(track_file)

def generate_simctl_command(points, speed_kmh=60, interval=0.1, distance=None, device="booted"):
    """Generate simctl location start command."""
    if len(points) < 2:
        raise ValueError("Need at least 2 waypoints for simctl location start")
    
    # Convert km/h to m/s
    speed_mps = speed_kmh / 3.6
    
    # Format waypoints as lat,lon pairs
    waypoint_strings = [f"{lat:.6f},{lon:.6f}" for lat, lon in points]
    
    # Build command
    cmd = ["xcrun", "simctl", "location", device, "start"]
    cmd.append(f"--speed={speed_mps:.2f}")
    
    if distance:
        cmd.append(f"--distance={distance}")
    else:
        cmd.append(f"--interval={interval}")
    
    cmd.extend(waypoint_strings)
    
    return cmd


def generate_devicectl_route(points, speed_kmh=60, interval=0.1, distance=None):
    """Generate a route for a physical device."""
    if len(points) < 2:
        raise ValueError("Need at least 2 waypoints for devicectl location route")

    route = {
        "mode": "distance" if distance is not None else "interval",
        "speed": speed_kmh / 3.6,
        "waypoints": [
            {"latitude": latitude, "longitude": longitude}
            for latitude, longitude in points
        ],
    }
    if distance is not None:
        route["distance"] = distance
    else:
        route["interval"] = interval
    return route


def generate_devicectl_command(route_file, device):
    return [
        "xcrun", "devicectl", "device", "simulate", "location", "route",
        "--device", device,
        "--route-file", str(route_file),
    ]


def generate_clear_command(device, physical_device=False):
    if physical_device:
        return [
            "xcrun", "devicectl", "device", "simulate", "location", "clear",
            "--device", device,
        ]
    return ["xcrun", "simctl", "location", device, "clear"]


def list_booted_simulators():
    try:
        result = subprocess.run(
            ["xcrun", "simctl", "list", "devices", "booted", "--json"],
            capture_output=True, text=True, check=True,
        )
        runtimes = json.loads(result.stdout).get("devices", {})
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError):
        return []

    return [
        (device["name"], device["udid"])
        for devices in runtimes.values()
        for device in devices
        if device.get("state") == "Booted"
    ]


def list_physical_devices():
    try:
        result = subprocess.run(
            ["xcrun", "devicectl", "list", "devices", "--quiet", "--json-output", "-"],
            capture_output=True, text=True, check=True,
        )
        devices = json.loads(result.stdout)["result"]["devices"]
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError, KeyError):
        return []

    physical = []
    for device in devices:
        hardware = device.get("hardwareProperties", {})
        connection = device.get("connectionProperties", {})
        if (hardware.get("reality") == "physical"
                and hardware.get("platform") in ("iOS", "iPadOS")
                and connection.get("pairingState") == "paired"):
            physical.append((
                device.get("deviceProperties", {}).get("name", hardware.get("udid")),
                hardware.get("udid"),
                connection.get("tunnelState", "unknown"),
            ))
    return physical


def choose_target(args):
    targets = [
        (f"{name} (simulator, {udid})", "device", udid)
        for name, udid in list_booted_simulators()
    ] + [
        (f"{name} (physical, {state}, {udid})", "physical_device", udid)
        for name, udid, state in list_physical_devices()
    ]

    if not targets:
        print("Error: No booted simulator or paired iPhone found", file=sys.stderr)
        print("  Boot a simulator, or connect and unlock your iPhone", file=sys.stderr)
        return False

    print("Available targets:")
    for index, (label, _, _) in enumerate(targets, start=1):
        print(f"  {index}. {label}")

    if not sys.stdin.isatty():
        print("\nRerun with one of:")
        for _, kind, udid in targets:
            flag = "--device" if kind == "device" else "--physical-device"
            print(f"  {flag} {udid}")
        return False

    while True:
        try:
            answer = input(f"Select target [1-{len(targets)}]: ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return False
        if answer.isdigit() and 1 <= int(answer) <= len(targets):
            _, kind, udid = targets[int(answer) - 1]
            setattr(args, kind, udid)
            return True
        print(f"Please enter a number between 1 and {len(targets)}")

def main():
    parser = argparse.ArgumentParser(
        description="Replay a GPX/KML/KMZ track on an iOS Simulator or physical device",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python ios_simulator_load_gpx.py test_route.gpx --speed 60 --interval 0.1
  python ios_simulator_load_gpx.py test_route.kmz --speed 80 --distance 10 --clear-first
  python ios_simulator_load_gpx.py test_route.kml --speed 50 --dry-run
  python ios_simulator_load_gpx.py test_route.gpx --physical-device "My iPhone" --speed 50
  python ios_simulator_load_gpx.py test_route.gpx --physical-device DEVICE_UDID --dry-run

Run without --device/--physical-device to choose from available targets.

Return a physical device to its real GPS location:
  xcrun devicectl device simulate location clear --device DEVICE_UDID
If you forget this you can always restart the device to restore the normal location.
        """
    )
    
    parser.add_argument('track_file', help='Input GPX, KML or KMZ file')
    parser.add_argument('--speed', type=float, default=60, 
                       help='Speed in km/h (default: 60)')
    parser.add_argument('--interval', type=float, default=0.1,
                       help='Update interval in seconds (default: 0.1)')
    parser.add_argument('--distance', type=float,
                       help='Update distance in meters (overrides --interval)')
    parser.add_argument('--device',
                       help="Target Simulator name/UDID or 'booted'")
    parser.add_argument('--physical-device', metavar='DEVICE',
                       help='Replay on a physical device using its name or UDID')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show command without executing (default: execute)')
    parser.add_argument('--clear-first', action='store_true',
                       help='Clear existing location before starting')
    
    args = parser.parse_args()

    if args.speed <= 0:
        parser.error("--speed must be greater than zero")
    if args.distance is not None and args.distance <= 0:
        parser.error("--distance must be greater than zero")
    if args.distance is None and args.interval <= 0:
        parser.error("--interval must be greater than zero")
    
    # Validate input file
    track_file = Path(args.track_file)
    if not track_file.exists():
        print(f"Error: track file '{track_file}' not found", file=sys.stderr)
        return 1

    if args.device is None and args.physical_device is None:
        if not choose_target(args):
            return 1
    
    try:
        # Extract waypoints
        points = extract_track_points(track_file)
        print(f"Extracted {len(points)} waypoints from {track_file}")
        
        if len(points) < 2:
            print("Error: Need at least 2 waypoints for location simulation", file=sys.stderr)
            return 1
        
        physical_device = args.physical_device is not None

        if physical_device:
            route = generate_devicectl_route(
                points,
                speed_kmh=args.speed,
                interval=args.interval,
                distance=args.distance,
            )
            print(f"\nTarget: physical device {args.physical_device}")
        else:
            cmd = generate_simctl_command(
                points,
                speed_kmh=args.speed,
                interval=args.interval,
                distance=args.distance,
                device=args.device,
            )
            print("\nGenerated simctl command:")
            print(shlex.join(cmd))
        
        # Calculate simulation info
        speed_mps = args.speed / 3.6
        total_distance = 0
        for i in range(1, len(points)):
            lat1, lon1 = points[i-1]
            lat2, lon2 = points[i]
            # Simple distance approximation
            total_distance += ((lat2-lat1)**2 + (lon2-lon1)**2)**0.5 * 111000  # rough conversion to meters
        
        duration = total_distance / speed_mps
        print(f"\nSimulation info:")
        print(f"  Speed: {args.speed} km/h ({speed_mps:.1f} m/s)")
        print(f"  Waypoints: {len(points)}")
        print(f"  Estimated distance: {total_distance/1000:.2f} km")
        print(f"  Estimated duration: {duration:.0f} seconds ({duration/60:.1f} minutes)")
        if args.distance:
            print(f"  Update distance: {args.distance}m")
        else:
            print(f"  Update interval: {args.interval}s")
        
        # Execute by default unless dry-run
        if args.dry_run:
            print("\n[DRY RUN] Command that would be executed:")
            if physical_device:
                print(json.dumps(route, indent=2))
                preview_cmd = generate_devicectl_command(
                    "<temporary-route-file.json>", args.physical_device
                )
                print(f"  {shlex.join(preview_cmd)}")
            else:
                print(f"  {shlex.join(cmd)}")
            if args.clear_first:
                clear_cmd = generate_clear_command(
                    args.physical_device if physical_device else args.device,
                    physical_device=physical_device,
                )
                print(f"  (would clear location first: {shlex.join(clear_cmd)})")
        else:
            print("\nExecuting command...")
            
            # Clear location first if requested
            if args.clear_first:
                clear_cmd = generate_clear_command(
                    args.physical_device if physical_device else args.device,
                    physical_device=physical_device,
                )
                print("Clearing existing location...")
                subprocess.run(clear_cmd, check=True)

            if physical_device:
                with tempfile.NamedTemporaryFile(
                    mode="w", suffix=".json", encoding="utf-8"
                ) as route_file:
                    json.dump(route, route_file)
                    route_file.flush()
                    cmd = generate_devicectl_command(
                        route_file.name, args.physical_device
                    )
                    result = subprocess.run(cmd, capture_output=True, text=True)
            else:
                result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✅ Location simulation started successfully!")
                if result.stdout.strip():
                    print(result.stdout.strip())
                if physical_device:
                    clear_cmd = generate_clear_command(
                        args.physical_device, physical_device=True
                    )
                    print(f"To return to real GPS: {shlex.join(clear_cmd)} (or restart the device)")
            else:
                print(f"❌ Error executing command:")
                print(result.stderr.strip())
                return 1
        
        return 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
