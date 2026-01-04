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

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


def load_country_polygons(polygons_file: Path) -> Dict[str, any]:
    """
    Load country polygons from packed_polygons.bin file.

    This is a placeholder - actual implementation would need to parse the binary format.
    For now, we'll use a simpler approach with DuckDB spatial functions.
    """
    # TODO: Implement actual polygon loading from packed_polygons.bin
    # For MVP, we can use a simplified approach or require pre-processed country boundaries
    logger.warning("Country polygon loading not yet implemented - using fallback method")
    return {}


def determine_country_from_coords(lat: float, lon: float, conn: duckdb.DuckDBPyConnection) -> str:
    """
    Determine which country a coordinate belongs to.

    This uses a simple approach for MVP - can be enhanced later.
    Returns country name or "Unknown" if not found.
    """
    # Simplified country detection for MVP
    # TODO: Use actual country polygons for accurate spatial join

    # For now, return a simplified country code based on rough lat/lon bounds
    # This is just for initial testing - real implementation needs proper spatial join
    if 40 < lat < 52 and -5 < lon < 10:
        return "France"
    elif 45 < lat < 48 and 5 < lon < 11:
        return "Switzerland"
    elif 43 < lat < 44 and 7 < lon < 8:
        return "Monaco"
    else:
        return "Unknown"


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


def process_parquet_streaming(parquet_url: str, output_dir: Path, batch_size: int = 100000):
    """
    Stream the Panoramax parquet file and write per-country binary files.

    Uses DuckDB to stream the large parquet file without loading it entirely into memory.
    """
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

    # Dictionary to accumulate points per country
    country_points: Dict[str, List[Tuple[float, float, str]]] = defaultdict(list)

    # Stream the parquet file in batches
    # Assuming parquet has columns: latitude, longitude, id (or similar)
    # Adjust column names based on actual Panoramax parquet schema
    query = f"""
        SELECT
            latitude as lat,
            longitude as lon,
            id as image_id
        FROM read_parquet('{parquet_url}')
        WHERE latitude IS NOT NULL AND longitude IS NOT NULL
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

                # Determine country
                country = determine_country_from_coords(lat, lon, conn)

                # Skip unknown countries for now (or save to separate file)
                if country != "Unknown":
                    country_points[country].append((lat, lon, str(image_id)))

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
        '--polygons',
        type=Path,
        help='Path to packed_polygons.bin file (optional, for accurate country detection)'
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
    logger.info(f"Batch size: {args.batch_size}")

    if args.polygons:
        logger.info(f"Country polygons: {args.polygons}")
        # TODO: Load and use country polygons for accurate spatial join
    else:
        logger.warning("No country polygons provided - using simplified country detection")

    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)

    # Process the parquet file
    process_parquet_streaming(args.input, args.output, args.batch_size)

    logger.info("Panoramax preprocessing complete!")


if __name__ == '__main__':
    main()
