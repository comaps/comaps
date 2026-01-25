#!/usr/bin/env python3
"""
Panoramax Preprocessor

Converts the global Panoramax geoparquet file into per-country binary files
for use in the map generator.

The script streams the large geoparquet file (20GB+) using DuckDB to avoid
loading everything into memory, performs a spatial join with country polygons,
and writes compact binary files for each country.

Binary Format:
  Header:
    uint32 version (=1)
    uint64 point_count
  Data (repeated point_count times):
    double lat (8 bytes)
    double lon (8 bytes)
    string image_id (length-prefixed: uint32 length + bytes)
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


def write_binary_file(output_path: Path, points: List[Tuple[float, float, str]]):
    """
    Write panoramax points to binary file.

    Format:
      Header:
        uint32 version = 1
        uint64 point_count
      Data:
        For each point:
          double lat
          double lon
          uint32 image_id_length
          bytes image_id
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, 'wb') as f:
        # Write header
        version = 1
        point_count = len(points)
        f.write(struct.pack('<I', version))  # uint32 version
        f.write(struct.pack('<Q', point_count))  # uint64 point_count

        # Write points
        for lat, lon, image_id in points:
            f.write(struct.pack('<d', lat))  # double lat
            f.write(struct.pack('<d', lon))  # double lon

            # Write image_id as length-prefixed string
            image_id_bytes = image_id.encode('utf-8')
            f.write(struct.pack('<I', len(image_id_bytes)))  # uint32 length
            f.write(image_id_bytes)  # bytes

    logger.info(f"Wrote {point_count} points to {output_path}")


def process_parquet_streaming(parquet_url: str, output_dir: Path, borders_dir: Path, batch_size: int = 100000):
    """
    Stream the Panoramax parquet file and write per-country binary files.

    Uses DuckDB to stream the large parquet file without loading it entirely into memory.
    Uses .poly files from borders_dir to categorize points into regions.
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
        logger.info(f"Parquet schema: {[col[0] for col in schema_result]}")
    except Exception as e:
        logger.warning(f"Could not read schema: {e}")

    # Dictionary to accumulate points per country
    country_points: Dict[str, List[Tuple[float, float, str]]] = defaultdict(list)

    # Stream the parquet file in batches
    # Geoparquet stores geometry as GEOMETRY type
    # Use DuckDB spatial functions to extract lat/lon
    query = f"""
        SELECT
            ST_Y(geometry) as lat,
            ST_X(geometry) as lon,
            id as image_id
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
                lat, lon, image_id = row

                # Find which region this point belongs to
                region = region_finder.find_region(lat, lon)

                # Only add points that fall within a defined region
                if region:
                    country_points[region].append((lat, lon, str(image_id)))

            # Periodically write to disk to avoid memory issues
            if batch_count % 10 == 0:
                for country, points in country_points.items():
                    if len(points) > 100000:  # Write if accumulated > 100k points
                        output_file = output_dir / f"{country}.panoramax"
                        # Append mode for incremental writing
                        # TODO: Implement append mode or accumulate all then write once
                        logger.info(f"Country {country} has {len(points)} points accumulated")

        logger.info(f"Finished processing {total_points} total points")
        logger.info(f"Countries found: {list(country_points.keys())}")

        # Write final output files
        for country, points in country_points.items():
            if points:
                output_file = output_dir / f"{country}.panoramax"
                write_binary_file(output_file, points)

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
