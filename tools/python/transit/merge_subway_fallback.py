#!/usr/bin/env python3
"""Restore missing transit networks from a last-known-good subway.json backup.

The subway validator excludes broken/bad networks from its output entirely.
This script detects networks that were present in the backup but absent in the
new run, and restores them so map coverage isn't lost for temporarily broken cities.

Usage:
    merge_subway_fallback.py <new_file> <backup_file> <output_file>

Writes a summary of restored cities to /tmp/subway_restored_cities.txt.
"""
import argparse
import json
import sys


RESTORED_CITIES_FILE = '/tmp/subway_restored_cities.txt'


def get_network_names(data):
    return {n['network'] for n in data.get('networks', [])}


def collect_stop_ids_for_network(network):
    stop_ids = set()
    for route in network.get('routes', []):
        for itinerary in route.get('itineraries', []):
            for stop_entry in itinerary.get('stops', []):
                stop_ids.add(stop_entry[0])
    return stop_ids


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_file', help='New subway.json from current validator run')
    parser.add_argument('backup_file', help='Last known good subway.json backup')
    parser.add_argument('output_file', help='Output path for merged subway.json')
    args = parser.parse_args()

    with open(args.new_file, 'r') as f:
        new_data = json.load(f)

    try:
        with open(args.backup_file, 'r') as f:
            backup_data = json.load(f)
    except FileNotFoundError:
        print('No backup file found, skipping fallback.')
        with open(args.output_file, 'w') as f:
            json.dump(new_data, f, ensure_ascii=False)
        return

    missing = get_network_names(backup_data) - get_network_names(new_data)

    if not missing:
        print('All cities present in new run, no fallback needed.')
        with open(args.output_file, 'w') as f:
            json.dump(new_data, f, ensure_ascii=False)
        return

    restored_names = sorted(missing)
    print(f'Restoring {len(restored_names)} cities from last-known-good backup: {", ".join(restored_names)}')

    # Collect stop IDs for all missing networks and add the networks back.
    restored_stop_ids = set()
    for network in backup_data.get('networks', []):
        if network['network'] in missing:
            new_data['networks'].append(network)
            restored_stop_ids |= collect_stop_ids_for_network(network)

    # Add stops for restored cities, skipping any stop IDs already in the new data.
    existing_stop_ids = {s['id'] for s in new_data.get('stops', [])}
    for stop in backup_data.get('stops', []):
        if stop['id'] in restored_stop_ids and stop['id'] not in existing_stop_ids:
            new_data['stops'].append(stop)
            existing_stop_ids.add(stop['id'])

    # Add transfers involving restored stops, skipping duplicates.
    existing_transfers = {(t[0], t[1]) for t in new_data.get('transfers', [])}
    for transfer in backup_data.get('transfers', []):
        if transfer[0] in restored_stop_ids or transfer[1] in restored_stop_ids:
            key = (transfer[0], transfer[1])
            if key not in existing_transfers:
                new_data['transfers'].append(transfer)
                existing_transfers.add(key)

    with open(args.output_file, 'w') as f:
        json.dump(new_data, f, ensure_ascii=False)

    summary = f'Restored from backup (validator marked bad): {", ".join(restored_names)}'
    with open(RESTORED_CITIES_FILE, 'w') as f:
        f.write(summary)
    print(f'Merged output written to {args.output_file}')


if __name__ == '__main__':
    main()
