#!/bin/bash
# Post-edit validation hook for Coldnb
# Runs after Claude edits/writes a file to catch errors immediately.

INPUT=$(cat)

FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path')

if [ -z "$FILE_PATH" ] || [ "$FILE_PATH" = "null" ]; then
    exit 0
fi

PROJECT_ROOT="/home/lucas/coldnb"
BACKEND_DIR="$PROJECT_ROOT/coldnb-backend"
FRONTEND_DIR="$PROJECT_ROOT/coldnb main/coldnb nextjs"

# --- Backend C files: incremental compile ---
if [[ "$FILE_PATH" == "$BACKEND_DIR"/src/*.c ]] || [[ "$FILE_PATH" == "$BACKEND_DIR"/src/**/*.c ]] || \
   [[ "$FILE_PATH" == "$BACKEND_DIR"/include/*.h ]] || [[ "$FILE_PATH" == "$BACKEND_DIR"/include/**/*.h ]]; then
    OUTPUT=$(cd "$BACKEND_DIR" && make 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]; then
        REASON=$(echo "$OUTPUT" | grep -E "error:" | head -5)
        jq -n --arg reason "Build failed after editing $FILE_PATH:
$REASON" '{
            "decision": "block",
            "reason": $reason
        }'
        exit 0
    fi
    exit 0
fi

# --- Frontend JS/JSX files: quick syntax check ---
if [[ "$FILE_PATH" == "$FRONTEND_DIR"/*.js ]] || [[ "$FILE_PATH" == "$FRONTEND_DIR"/**/*.js ]] || \
   [[ "$FILE_PATH" == "$FRONTEND_DIR"/*.jsx ]] || [[ "$FILE_PATH" == "$FRONTEND_DIR"/**/*.jsx ]]; then
    OUTPUT=$(node --check "$FILE_PATH" 2>&1)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]; then
        jq -n --arg reason "Syntax error in $FILE_PATH:
$OUTPUT" '{
            "decision": "block",
            "reason": $reason
        }'
        exit 0
    fi
    exit 0
fi

# --- SCSS files: check file is not empty if it was written ---
if [[ "$FILE_PATH" == *.scss ]]; then
    if [ ! -s "$FILE_PATH" ]; then
        jq -n --arg reason "SCSS file is empty: $FILE_PATH" '{
            "decision": "block",
            "reason": $reason
        }'
        exit 0
    fi
    exit 0
fi

exit 0
