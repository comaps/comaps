#!/usr/bin/env python3
"""
Panoramax Preprocessor

Converts the global Panoramax geoparquet file into per-country binary files
for use in the map generator.

The script streams the large geoparquet file (20GB+) using DuckDB to avoid
loading everything into memory, performs a spatial join with country polygons,
and writes compact binary files for each country.

Binary Format (version 3):
  Header:
    uint32 version (=3)
    uint64 point_count
    uint64 sequence_count
  Sequences (repeated sequence_count times):
    string sequence_id (length-prefixed: uint32 length + bytes)
    uint64 point_count_in_sequence
    Points (repeated point_count_in_sequence times, ordered by datetime):
      double lat (8 bytes)
      double lon (8 bytes)
      string image_id (length-prefixed: uint32 length + bytes)
      int16 azimuth (-1 for null, 0-359 for camera heading direction)
      int16 field_of_view (-1 for null, otherwise FOV in degrees; 360 = spherical)
"""

import argparse
import logging
import struct
import sys
from pathlib import Path
from typing import Dict, List, Tuple
from collections import defaultdict

try:
    import duckdb
except ImportError:
    print("Error: duckdb is required. Install with: pip install duckdb", file=sys.stderr)
    sys.exit(1)

try:
    from shapely.geometry import Point, Polygon, MultiPolygon
    from shapely.strtree import STRtree
except ImportError:
    print("Error: shapely is required. Install with: pip install shapely", file=sys.stderr)
    sys.exit(1)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


def parse_poly_file(poly_path: Path) -> MultiPolygon:
    """
    Parse an Osmosis .poly file and return a Shapely MultiPolygon.

    .poly format:
      Line 1: Region name
      Section N: (numbered 1, 2, 3...)
        lon lat (pairs of coordinates)
        ...
        END
    """
    with open(poly_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    polygons = []
    current_coords = []
    in_section = False

    for line in lines[1:]:  # Skip first line (region name)
        line = line.strip()

        if not line:
            continue

        if line.upper() == 'END':
            if current_coords:
                # Close the polygon if needed
                if current_coords[0] != current_coords[-1]:
                    current_coords.append(current_coords[0])

                # Create polygon (need at least 3 points + closing point)
                if len(current_coords) >= 4:
                    try:
                        poly = Polygon(current_coords)

                        # If polygon is invalid, try to fix it
                        if not poly.is_valid:
                            # Try buffer(0) trick to fix self-intersections
                            poly = poly.buffer(0)

                        # Only accept if it's now valid and is a Polygon or MultiPolygon
                        if poly.is_valid and not poly.is_empty:
                            if poly.geom_type == 'Polygon':
                                polygons.append(poly)
                            elif poly.geom_type == 'MultiPolygon':
                                # Split multipolygon into individual polygons
                                polygons.extend(poly.geoms)
                        else:
                            logger.debug(f"Skipping invalid section in {poly_path.name}")

                    except Exception as e:
                        logger.debug(f"Error creating polygon in {poly_path.name}: {e}")

                current_coords = []
                in_section = False
            continue

        # Try to parse as section number
        try:
            int(line)
            in_section = True
            continue
        except ValueError:
            pass

        # Parse coordinate pair
        if in_section:
            parts = line.split()
            if len(parts) >= 2:
                try:
                    lon = float(parts[0])
                    lat = float(parts[1])
                    current_coords.append((lon, lat))
                except ValueError:
                    pass

    if not polygons:
        logger.warning(f"No valid polygons found in {poly_path.name}")
        return None

    if len(polygons) == 1:
        return MultiPolygon([polygons[0]])
    else:
        return MultiPolygon(polygons)


def load_country_polygons(borders_dir: Path) -> Dict[str, MultiPolygon]:
    """
    Load all .poly files from the borders directory.

    Returns a dict mapping region name (without .poly extension) to MultiPolygon.
    """
    logger.info(f"Loading .poly files from {borders_dir}")

    poly_files = list(borders_dir.glob("*.poly"))
    logger.info(f"Found {len(poly_files)} .poly files")

    polygons = {}

    for poly_file in poly_files:
        region_name = poly_file.stem  # Filename without .poly extension

        try:
            multi_polygon = parse_poly_file(poly_file)
            if multi_polygon:
                polygons[region_name] = multi_polygon
        except Exception as e:
            logger.error(f"Error parsing {poly_file.name}: {e}")
            continue

    logger.info(f"Successfully loaded {len(polygons)} region polygons")
    return polygons


class RegionFinder:
    """
    Efficient spatial index for finding which region a point belongs to.
    Uses Shapely's STRtree for fast spatial queries.
    """
    def __init__(self, regions: Dict[str, MultiPolygon]):
        logger.info("Building spatial index for region lookup...")

        self.regions = regions
        self.region_names = []
        self.geometries = []

        for region_name, multi_polygon in regions.items():
            self.region_names.append(region_name)
            self.geometries.append(multi_polygon)

        # Build R-tree spatial index for fast lookups
        self.tree = STRtree(self.geometries)

        logger.info(f"Spatial index built with {len(self.geometries)} regions")

    def find_region(self, lat: float, lon: float) -> str:
        """
        Find which region a coordinate belongs to.

        Returns region name or None if not found.
        """
        point = Point(lon, lat)  # Note: Shapely uses (x, y) = (lon, lat)

        # Query the spatial index for candidate polygons
        candidates = self.tree.query(point)

        # Check each candidate to see if point is actually inside
        for idx in candidates:
            if self.geometries[idx].contains(point):
                return self.region_names[idx]

        return None


def write_binary_file(output_path: Path, sequences: Dict[str, List[Tuple[float, float, str, int, int]]]):
    """
    Write panoramax sequences to binary file.

    Format (version 3):
      Header:
        uint32 version = 3
        uint64 total_point_count
        uint64 sequence_count
      Sequences:
        For each sequence:
          uint32 sequence_id_length
          bytes sequence_id
          uint64 point_count_in_sequence
          For each point:
            double lat
            double lon
            uint32 image_id_length
            bytes image_id
            int16 azimuth (-1 for null, 0-359 for camera heading)
            int16 field_of_view (-1 for null, otherwise FOV degrees)
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)

    total_points = sum(len(pts) for pts in sequences.values())
    sequence_count = len(sequences)

    with open(output_path, 'wb') as f:
        # Write header
        version = 3
        f.write(struct.pack('<I', version))  # uint32 version
        f.write(struct.pack('<Q', total_points))  # uint64 total_point_count
        f.write(struct.pack('<Q', sequence_count))  # uint64 sequence_count

        # Write sequences
        for sequence_id, points in sequences.items():
            # Write sequence_id as length-prefixed string
            sequence_id_bytes = sequence_id.encode('utf-8')
            f.write(struct.pack('<I', len(sequence_id_bytes)))  # uint32 length
            f.write(sequence_id_bytes)  # bytes

            # Write point count in this sequence
            f.write(struct.pack('<Q', len(points)))  # uint64 point_count

            # Write points (already sorted by datetime)
            for lat, lon, image_id, azimuth, fov in points:
                f.write(struct.pack('<d', lat))  # double lat
                f.write(struct.pack('<d', lon))  # double lon

                # Write image_id as length-prefixed string
                image_id_bytes = image_id.encode('utf-8')
                f.write(struct.pack('<I', len(image_id_bytes)))  # uint32 length
                f.write(image_id_bytes)  # bytes

                # Write azimuth and field_of_view as int16 (-1 for null)
                f.write(struct.pack('<h', azimuth if azimuth is not None else -1))
                f.write(struct.pack('<h', fov if fov is not None else -1))

    logger.info(f"Wrote {total_points} points in {sequence_count} sequences to {output_path}")


def process_parquet_streaming(parquet_url: str, output_dir: Path, borders_dir: Path, batch_size: int = 100000):
    """
    Stream the Panoramax parquet file and write per-country binary files.

    Uses DuckDB to stream the large parquet file without loading it entirely into memory.
    Uses .poly files from borders_dir to categorize points into regions.
    Points are grouped by sequence (collection) and sorted by datetime within each sequence.
    """
    # Load region polygons and build spatial index
    regions = load_country_polygons(borders_dir)
    if not regions:
        logger.error("No regions loaded - cannot process panoramax data")
        return

    region_finder = RegionFinder(regions)

    conn = duckdb.connect(database=':memory:')

    # Enable httpfs extension for remote file access
    try:
        conn.execute("INSTALL httpfs;")
        conn.execute("LOAD httpfs;")
    except Exception as e:
        logger.warning(f"Could not load httpfs extension: {e}")

    # Install spatial extension for future country boundary support
    try:
        conn.execute("INSTALL spatial;")
        conn.execute("LOAD spatial;")
    except Exception as e:
        logger.warning(f"Could not load spatial extension: {e}")

    logger.info(f"Reading parquet file: {parquet_url}")

    # First, inspect the schema to understand the columns
    try:
        schema_result = conn.execute(f"DESCRIBE SELECT * FROM read_parquet('{parquet_url}') LIMIT 0").fetchall()
        columns = [col[0] for col in schema_result]
        logger.info(f"Parquet schema: {columns}")

        # Check if key fields exist
        has_collection = 'collection' in columns
        has_datetime = 'datetime' in columns
        has_azimuth = 'view:azimuth' in columns
        has_interior_orientation = 'pers:interior_orientation' in columns
        logger.info(f"Has collection: {has_collection}, datetime: {has_datetime}, azimuth: {has_azimuth}, interior_orientation: {has_interior_orientation}")
    except Exception as e:
        logger.warning(f"Could not read schema: {e}")
        has_collection = False
        has_datetime = False
        has_azimuth = False
        has_interior_orientation = False

    # Dictionary to accumulate sequences per country
    # Structure: {country: {sequence_id: [(lat, lon, image_id, datetime, azimuth, fov), ...]}}
    country_sequences: Dict[str, Dict[str, List[Tuple[float, float, str, str, int, int]]]] = defaultdict(lambda: defaultdict(list))

    # Build azimuth and FOV column expressions
    azimuth_col = '"view:azimuth"' if has_azimuth else 'NULL'
    fov_col = '"pers:interior_orientation".field_of_view' if has_interior_orientation else 'NULL'

    # Stream the parquet file in batches
    # Geoparquet stores geometry as GEOMETRY type
    # Use DuckDB spatial functions to extract lat/lon
    # Also extract collection (sequence_id), datetime, azimuth, and field_of_view
    if has_collection and has_datetime:
        query = f"""
            SELECT
                ST_Y(geometry) as lat,
                ST_X(geometry) as lon,
                id as image_id,
                collection as sequence_id,
                datetime,
                {azimuth_col} as azimuth,
                {fov_col} as fov
            FROM read_parquet('{parquet_url}')
            WHERE geometry IS NOT NULL
            ORDER BY collection, datetime
        """
    elif has_collection:
        query = f"""
            SELECT
                ST_Y(geometry) as lat,
                ST_X(geometry) as lon,
                id as image_id,
                collection as sequence_id,
                NULL as datetime,
                {azimuth_col} as azimuth,
                {fov_col} as fov
            FROM read_parquet('{parquet_url}')
            WHERE geometry IS NOT NULL
            ORDER BY collection
        """
    else:
        # Fallback: no sequence info, use image_id as sequence_id (single-point sequences)
        query = f"""
            SELECT
                ST_Y(geometry) as lat,
                ST_X(geometry) as lon,
                id as image_id,
                id as sequence_id,
                NULL as datetime,
                {azimuth_col} as azimuth,
                {fov_col} as fov
            FROM read_parquet('{parquet_url}')
            WHERE geometry IS NOT NULL
        """

    try:
        result = conn.execute(query)

        batch_count = 0
        total_points = 0

        while True:
            batch = result.fetchmany(batch_size)
            if not batch:
                break

            batch_count += 1
            batch_size_actual = len(batch)
            total_points += batch_size_actual

            logger.info(f"Processing batch {batch_count}: {batch_size_actual} points (total: {total_points})")

            for row in batch:
                lat, lon, image_id, sequence_id, datetime_val, azimuth, fov = row

                # Find which region this point belongs to
                region = region_finder.find_region(lat, lon)

                # Only add points that fall within a defined region
                if region and sequence_id:
                    # Convert azimuth and fov to int (or None)
                    azimuth_int = int(azimuth) if azimuth is not None else None
                    fov_int = int(fov) if fov is not None else None
                    country_sequences[region][str(sequence_id)].append(
                        (lat, lon, str(image_id), str(datetime_val) if datetime_val else "", azimuth_int, fov_int)
                    )

            # Log progress
            if batch_count % 10 == 0:
                total_sequences = sum(len(seqs) for seqs in country_sequences.values())
                logger.info(f"Accumulated {total_sequences} sequences across {len(country_sequences)} countries")

        logger.info(f"Finished processing {total_points} total points")
        logger.info(f"Countries found: {list(country_sequences.keys())}")

        # Write final output files
        for country, sequences in country_sequences.items():
            if sequences:
                # Sort points within each sequence by datetime (already mostly sorted from query)
                sorted_sequences: Dict[str, List[Tuple[float, float, str, int, int]]] = {}
                for seq_id, points in sequences.items():
                    # Sort by datetime (4th element), then keep lat, lon, image_id, azimuth, fov
                    sorted_points = sorted(points, key=lambda p: p[3])
                    # Output: (lat, lon, image_id, azimuth, fov) - remove datetime
                    sorted_sequences[seq_id] = [(p[0], p[1], p[2], p[4], p[5]) for p in sorted_points]

                output_file = output_dir / f"{country}.panoramax"
                write_binary_file(output_file, sorted_sequences)

    except Exception as e:
        logger.error(f"Error processing parquet: {e}")
        raise

    finally:
        conn.close()


def main():
    parser = argparse.ArgumentParser(
        description="Convert Panoramax geoparquet to per-country binary files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    parser.add_argument(
        '--input',
        default='https://api.panoramax.xyz/data/geoparquet/panoramax.parquet',
        help='Path or URL to Panoramax geoparquet file (default: official Panoramax URL)'
    )

    parser.add_argument(
        '--output',
        type=Path,
        required=True,
        help='Output directory for per-country .panoramax files'
    )

    parser.add_argument(
        '--borders-dir',
        type=Path,
        default=Path(__file__).parent.parent.parent.parent / 'data' / 'borders',
        help='Path to directory containing .poly border files (default: <repo>/data/borders)'
    )

    parser.add_argument(
        '--batch-size',
        type=int,
        default=100000,
        help='Number of rows to process per batch (default: 100000)'
    )

    args = parser.parse_args()

    logger.info("Panoramax Preprocessor starting")
    logger.info(f"Input: {args.input}")
    logger.info(f"Output directory: {args.output}")
    logger.info(f"Borders directory: {args.borders_dir}")
    logger.info(f"Batch size: {args.batch_size}")

    # Verify borders directory exists
    if not args.borders_dir.exists():
        logger.error(f"Borders directory not found: {args.borders_dir}")
        sys.exit(1)

    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)

    # Process the parquet file
    process_parquet_streaming(args.input, args.output, args.borders_dir, args.batch_size)

    logger.info("Panoramax preprocessing complete!")


if __name__ == '__main__':
    main()
