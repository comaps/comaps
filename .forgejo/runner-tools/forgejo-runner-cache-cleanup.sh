#!/usr/bin/env bash
#
# forgejo-runner-cache-cleanup.sh
#
# Clean up the Forgejo Actions runner's actcache to reclaim disk space.
#
# actcache layout:
#   actcache/
#     bolt.db                  – BoltDB metadata
#     cache/                   – content-addressable blob store
#       00/ 01/ .. ff/         – 256 hex-prefix shard directories
#         <id>                 – numbered blob files (cached tarballs)
#
# Usage:
#   ./forgejo-runner-cache-cleanup.sh                            # Summary overview
#   ./forgejo-runner-cache-cleanup.sh --detail                   # Per-file listing
#   ./forgejo-runner-cache-cleanup.sh --delete --older-than 14   # Delete blobs >14d old
#   ./forgejo-runner-cache-cleanup.sh --wipe                     # Nuke everything

set -euo pipefail

RUNNER_DATA_DIR="${RUNNER_DATA_DIR:-./data}"

DELETE=false
WIPE=false
OLDER_THAN_DAYS=0
SHOW_DETAIL=false
TOP_N=0
ALSO_CLEAN_ACT=false
YES=false

usage() {
    cat <<'EOF'
Usage: forgejo-runner-cache-cleanup.sh [OPTIONS]

Inspect and clean the Forgejo runner's actcache (actions/cache blob store).

Options:
  --detail              List every individual blob file
  --delete              Delete blobs matching filter criteria
  --wipe                Delete entire actcache directory (prompts for confirmation)
  --yes                 Skip confirmation prompt for --wipe
  --older-than DAYS     Only target blobs modified more than DAYS days ago
  --top N               Only show the top N largest blobs (with --detail)
  --clean-act           Also clean act working directories (.cache/act/)
  --data-dir PATH       Runner data directory (default: ./data)
  -h, --help            Show this help

Environment:
  RUNNER_DATA_DIR       Same as --data-dir

Examples:
  ./forgejo-runner-cache-cleanup.sh --data-dir /opt/forgejo-runner-org/data
  ./forgejo-runner-cache-cleanup.sh --detail --top 20
  ./forgejo-runner-cache-cleanup.sh --delete --older-than 7
  ./forgejo-runner-cache-cleanup.sh --wipe --yes
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --detail)       SHOW_DETAIL=true; shift ;;
        --delete)       DELETE=true; shift ;;
        --wipe)         WIPE=true; shift ;;
        --yes|-y)       YES=true; shift ;;
        --older-than)   OLDER_THAN_DAYS="$2"; shift 2 ;;
        --top)          TOP_N="$2"; shift 2 ;;
        --clean-act)    ALSO_CLEAN_ACT=true; shift ;;
        --data-dir)     RUNNER_DATA_DIR="$2"; shift 2 ;;
        -h|--help)      usage ;;
        *)              echo "Unknown option: $1"; usage ;;
    esac
done

# ── Helpers ──────────────────────────────────────────────────────────────────

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; DIM='\033[2m'; BOLD='\033[1m'; NC='\033[0m'

info()  { echo -e "${BLUE}ℹ${NC}  $*"; }
ok()    { echo -e "${GREEN}✓${NC}  $*"; }
warn()  { echo -e "${YELLOW}⚠${NC}  $*"; }
err()   { echo -e "${RED}✗${NC}  $*" >&2; }

human_size() {
    local bytes="${1:-0}"
    [[ -z "$bytes" ]] && bytes=0
    if [[ $bytes -ge 1073741824 ]]; then
        printf "%.2f GB" "$(echo "scale=2; $bytes / 1073741824" | bc)"
    elif [[ $bytes -ge 1048576 ]]; then
        printf "%.1f MB" "$(echo "scale=1; $bytes / 1048576" | bc)"
    elif [[ $bytes -ge 1024 ]]; then
        printf "%.1f KB" "$(echo "scale=1; $bytes / 1024" | bc)"
    else
        printf "%d B" "$bytes"
    fi
}

# ── Resolve paths ────────────────────────────────────────────────────────────

RUNNER_DATA_DIR="$(realpath "$RUNNER_DATA_DIR" 2>/dev/null || echo "$RUNNER_DATA_DIR")"
ACTCACHE_DIR="${RUNNER_DATA_DIR}/.cache/actcache"

# Check config.yml for custom cache dir
CONFIG_FILE="${RUNNER_DATA_DIR}/config.yml"
if [[ -f "$CONFIG_FILE" ]]; then
    custom_dir=$(grep -A5 '^cache:' "$CONFIG_FILE" 2>/dev/null \
        | grep '^\s*dir:' 2>/dev/null | head -1 \
        | sed 's/.*dir:\s*//' | tr -d '"'"'" | xargs 2>/dev/null || true)
    if [[ -n "${custom_dir:-}" && "${custom_dir}" != "" ]]; then
        if [[ "$custom_dir" == /* ]]; then
            ACTCACHE_DIR="${RUNNER_DATA_DIR}${custom_dir#/data}"
        else
            ACTCACHE_DIR="${RUNNER_DATA_DIR}/${custom_dir}"
        fi
        info "Custom cache dir from config.yml: ${ACTCACHE_DIR}"
    fi
fi

# The actual blob store is inside cache/ subdirectory
CACHE_STORE="${ACTCACHE_DIR}/cache"

if [[ ! -d "$ACTCACHE_DIR" ]]; then
    err "actcache directory not found: ${ACTCACHE_DIR}"
    echo "  Set --data-dir to the directory mounted as /data in your runner container."
    exit 1
fi

if [[ ! -d "$CACHE_STORE" ]]; then
    err "cache store not found: ${CACHE_STORE}"
    echo "  Expected: ${ACTCACHE_DIR}/cache/{00..ff}/<blobs>"
    exit 1
fi

# ── Gather all blobs ─────────────────────────────────────────────────────────
# Use find + stat in one pass, output: mtime_epoch|size_bytes|filepath

NOW=$(date +%s)
CUTOFF=0
if [[ "$OLDER_THAN_DAYS" -gt 0 ]]; then
    CUTOFF=$((NOW - OLDER_THAN_DAYS * 86400))
fi

TMPFILE=$(mktemp)
trap 'rm -f "$TMPFILE" "${TMPFILE}."* 2>/dev/null' EXIT

# Collect all blob files with stat info.
# Format: epoch_mtime|bytes|bucket|blobid|fullpath
find "$CACHE_STORE" -type f -printf '%T@|%s|%h|%f|%p\n' 2>/dev/null \
    | sed 's/\.[0-9]*|/|/' > "$TMPFILE" || true
    # strip fractional seconds from epoch

TOTAL_BLOBS=$(wc -l < "$TMPFILE" | tr -d ' ')
TOTAL_BLOBS="${TOTAL_BLOBS:-0}"

if [[ "$TOTAL_BLOBS" -eq 0 ]]; then
    echo ""
    info "No cache blobs found in ${CACHE_STORE}"
    exit 0
fi

TOTAL_SIZE=$(awk -F'|' '{s+=$2} END {print s+0}' "$TMPFILE")
DB_SIZE=0
for dbf in "${ACTCACHE_DIR}/bolt.db" "${ACTCACHE_DIR}/cache.db"; do
    if [[ -f "$dbf" ]]; then
        DB_SIZE=$(stat -c %s "$dbf" 2>/dev/null || echo 0)
        break
    fi
done

# ── Header ───────────────────────────────────────────────────────────────────

echo ""
echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}  Forgejo Runner — actcache Cleanup${NC}"
echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
echo "  actcache dir  : ${ACTCACHE_DIR}"
echo "  blob store    : ${CACHE_STORE}/"
echo "  metadata DB   : $(human_size "$DB_SIZE")"
echo "  total blobs   : ${TOTAL_BLOBS} files, $(human_size "$TOTAL_SIZE")"
echo -n "  mode          : "
if $WIPE; then
    echo -e "${RED}WIPE${NC}"
elif $DELETE; then
    echo -e "${RED}DELETE${NC}"
else
    echo -e "${GREEN}DRY-RUN${NC}"
fi
if [[ "$OLDER_THAN_DAYS" -gt 0 ]]; then
    echo "  age filter    : older than ${OLDER_THAN_DAYS} days"
fi
echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
echo ""

# ── Wipe mode ────────────────────────────────────────────────────────────────

if $WIPE; then
    wipe_total=$(du -sb "$ACTCACHE_DIR" 2>/dev/null | awk '{print $1}' || true)
    wipe_total="${wipe_total:-0}"
    echo -e "  ${YELLOW}This will delete the ENTIRE actcache directory:${NC}"
    echo "    ${ACTCACHE_DIR}  ($(human_size "$wipe_total"))"
    echo ""
    echo "  The runner will recreate it on next workflow run."
    echo ""

    if $YES; then
        confirm="yes"
    else
        read -rp "  Type 'yes' to confirm: " confirm
    fi

    if [[ "$confirm" == "yes" ]]; then
        rm -rf "$ACTCACHE_DIR"
        mkdir -p "$ACTCACHE_DIR"
        echo ""
        ok "Wiped actcache. Freed $(human_size "$wipe_total")."
        echo -e "  ${DIM}Restart your runner to reinitialize the cache server.${NC}"
    else
        warn "Aborted."
    fi

    if $ALSO_CLEAN_ACT; then
        ACT_DIR="${RUNNER_DATA_DIR}/.cache/act"
        if [[ -d "$ACT_DIR" ]]; then
            act_size=$(du -sb "$ACT_DIR" 2>/dev/null | awk '{print $1}' || true)
            act_size="${act_size:-0}"
            rm -rf "$ACT_DIR"
            mkdir -p "$ACT_DIR"
            ok "Wiped act working directories. Freed $(human_size "$act_size")."
        fi
    fi
    exit 0
fi

# ── Age distribution ─────────────────────────────────────────────────────────

echo -e "${BOLD}── Age distribution ───────────────────────────────────────────────${NC}"
echo ""

age_1d=0;  size_1d=0
age_7d=0;  size_7d=0
age_30d=0; size_30d=0
age_90d=0; size_90d=0
age_old=0; size_old=0

while IFS='|' read -r mtime fsize _bucket _blobid _path; do
    age=$(( (NOW - mtime) / 86400 ))
    if [[ $age -le 1 ]]; then
        age_1d=$((age_1d + 1)); size_1d=$((size_1d + fsize))
    elif [[ $age -le 7 ]]; then
        age_7d=$((age_7d + 1)); size_7d=$((size_7d + fsize))
    elif [[ $age -le 30 ]]; then
        age_30d=$((age_30d + 1)); size_30d=$((size_30d + fsize))
    elif [[ $age -le 90 ]]; then
        age_90d=$((age_90d + 1)); size_90d=$((size_90d + fsize))
    else
        age_old=$((age_old + 1)); size_old=$((size_old + fsize))
    fi
done < "$TMPFILE"

printf "  ${GREEN}%-14s${NC}  %5d blobs   %12s\n" "≤ 1 day"  "$age_1d"  "$(human_size $size_1d)"
printf "  ${GREEN}%-14s${NC}  %5d blobs   %12s\n" "2–7 days"  "$age_7d"  "$(human_size $size_7d)"
printf "  ${NC}%-14s${NC}  %5d blobs   %12s\n"    "8–30 days"  "$age_30d" "$(human_size $size_30d)"
printf "  ${YELLOW}%-14s${NC}  %5d blobs   %12s\n" "31–90 days" "$age_90d" "$(human_size $size_90d)"
printf "  ${RED}%-14s${NC}  %5d blobs   %12s\n"    "> 90 days"  "$age_old" "$(human_size $size_old)"
echo ""

# ── Top buckets by size ──────────────────────────────────────────────────────

echo -e "${BOLD}── Top 15 buckets by size ─────────────────────────────────────────${NC}"
echo ""

# Aggregate size per bucket
awk -F'|' '{
    bucket = $3
    sub(/.*\//, "", bucket)  # extract just the hex dir name
    sizes[bucket] += $2
    counts[bucket]++
}
END {
    for (b in sizes) print sizes[b] "|" counts[b] "|" b
}' "$TMPFILE" | sort -t'|' -k1 -rn | head -15 | while IFS='|' read -r bsize bcount bname; do
    pct=0
    if [[ $TOTAL_SIZE -gt 0 ]]; then
        pct=$(echo "scale=1; $bsize * 100 / $TOTAL_SIZE" | bc)
    fi
    printf "  ${BOLD}%-4s${NC}  %12s  %4d blobs  %5s%%\n" \
        "$bname" "$(human_size "$bsize")" "$bcount" "$pct"
done

echo ""

# ── Per-blob detail (optional) ───────────────────────────────────────────────

if $SHOW_DETAIL; then
    echo -e "${BOLD}── Blob detail (sorted by size, largest first) ────────────────────${NC}"
    echo ""
    printf "  ${DIM}%-6s  %-4s  %12s  %6s  %-19s  %s${NC}\n" \
        "BLOB" "SHARD" "SIZE" "AGE" "MODIFIED" "PATH"
    printf "  ${DIM}%-6s  %-4s  %12s  %6s  %-19s  %s${NC}\n" \
        "─────" "────" "────────────" "──────" "───────────────────" "────────────"

    sort -t'|' -k2 -rn "$TMPFILE" | {
        shown=0
        while IFS='|' read -r mtime fsize bucket blobid fullpath; do
            if [[ "$TOP_N" -gt 0 ]]; then
                shown=$((shown + 1))
                if [[ $shown -gt $TOP_N ]]; then
                    continue
                fi
            fi
            age=$(( (NOW - mtime) / 86400 ))
            bname="${bucket##*/}"
            moddate=$(date -d "@${mtime}" '+%Y-%m-%d %H:%M' 2>/dev/null \
                || date -r "$mtime" '+%Y-%m-%d %H:%M' 2>/dev/null \
                || echo "unknown")

            if [[ $age -gt 90 ]]; then
                color="$RED"
            elif [[ $age -gt 30 ]]; then
                color="$YELLOW"
            else
                color="$NC"
            fi
            printf "  ${color}%-6s  %-4s  %12s  %5dd  %-19s${NC}  ${DIM}%s${NC}\n" \
                "$blobid" "$bname" "$(human_size "$fsize")" "$age" "$moddate" "$fullpath"
        done

        if [[ "$TOP_N" -gt 0 && $shown -gt $TOP_N ]]; then
            remaining=$((shown - TOP_N))
            echo -e "  ${DIM}... and ${remaining} more blobs${NC}"
        fi
    }
    echo ""
fi

# ── Delete mode ──────────────────────────────────────────────────────────────

if $DELETE; then
    echo -e "${BOLD}── Deleting matching blobs ─────────────────────────────────────────${NC}"
    echo ""

    del_count=0
    del_size=0
    skip_count=0

    while IFS='|' read -r mtime fsize bucket blobid fullpath; do
        age=$(( (NOW - mtime) / 86400 ))

        # Age filter: skip blobs newer than cutoff
        if [[ "$CUTOFF" -gt 0 && "$mtime" -gt "$CUTOFF" ]]; then
            skip_count=$((skip_count + 1))
            continue
        fi

        rm -f "$fullpath"
        del_count=$((del_count + 1))
        del_size=$((del_size + fsize))

        # Print progress every 50 deletions
        if [[ $((del_count % 50)) -eq 0 ]]; then
            printf "\r  Deleted %d blobs ($(human_size $del_size))..." "$del_count"
        fi
    done < "$TMPFILE"

    # Clear progress line
    if [[ $del_count -ge 50 ]]; then
        printf "\r%-60s\r" " "
    fi

    echo ""
    echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
    echo -e "  ${GREEN}Deleted : ${del_count} blobs, $(human_size "$del_size")${NC}"
    if [[ $skip_count -gt 0 ]]; then
        echo "  Skipped : ${skip_count} blobs (newer than ${OLDER_THAN_DAYS} days)"
    fi
    echo ""
    echo -e "  ${DIM}Deleted blobs will cause cache misses; workflows will repopulate."
    if [[ $del_count -gt 20 ]]; then
        echo -e "  Consider --wipe if you deleted many entries (deletes bolt.db too).${NC}"
    fi
    echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"

# ── Dry-run summary ──────────────────────────────────────────────────────────

else
    # Calculate what --older-than would match
    match_count=0
    match_size=0
    if [[ "$CUTOFF" -gt 0 ]]; then
        while IFS='|' read -r mtime fsize _b _id _p; do
            if [[ "$mtime" -le "$CUTOFF" ]]; then
                match_count=$((match_count + 1))
                match_size=$((match_size + fsize))
            fi
        done < "$TMPFILE"
    else
        match_count=$TOTAL_BLOBS
        match_size=$TOTAL_SIZE
    fi

    echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
    echo "  Total          : ${TOTAL_BLOBS} blobs, $(human_size "$TOTAL_SIZE")"
    if [[ "$CUTOFF" -gt 0 ]]; then
        echo "  Would delete   : ${match_count} blobs, $(human_size "$match_size")"
        would_remain=$((TOTAL_SIZE - match_size))
        echo "  Would remain   : $((TOTAL_BLOBS - match_count)) blobs, $(human_size "$would_remain")"
    fi
    echo ""
    if [[ $match_count -gt 0 ]]; then
        echo -e "  ${YELLOW}This was a dry run.${NC} To delete:"
        if [[ "$OLDER_THAN_DAYS" -gt 0 ]]; then
            echo -e "    ${BOLD}$0 --delete --older-than ${OLDER_THAN_DAYS}${NC}"
        else
            echo -e "    ${BOLD}$0 --delete${NC}"
        fi
        echo -e "  Or wipe everything:"
        echo -e "    ${BOLD}$0 --wipe${NC}"
    fi
    echo -e "${BOLD}═══════════════════════════════════════════════════════════════════${NC}"
fi

# ── Also clean act workspaces ────────────────────────────────────────────────

if $ALSO_CLEAN_ACT; then
    ACT_DIR="${RUNNER_DATA_DIR}/.cache/act"
    if [[ -d "$ACT_DIR" ]]; then
        act_size=$(du -sb "$ACT_DIR" 2>/dev/null | awk '{print $1}' || true)
        act_size="${act_size:-0}"
        if [[ "$act_size" -gt 0 ]]; then
            echo ""
            if $DELETE || $WIPE; then
                rm -rf "$ACT_DIR"
                mkdir -p "$ACT_DIR"
                ok "Cleaned act working directories. Freed $(human_size "$act_size")."
            else
                info "act working directories: $(human_size "$act_size") (use --clean-act --delete to remove)"
            fi
        fi
    fi
fi
