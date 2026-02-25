#!/bin/bash
# Coldnb Development Restart Script
# Stops running servers, cleans cache, rebuilds, and starts fresh
# Saves last 300 lines of logs before each restart

set -e

PROJECT_ROOT="/home/lucas/coldnb"
BACKEND_DIR="$PROJECT_ROOT/coldnb-backend"
FRONTEND_DIR="$PROJECT_ROOT/coldnb main/coldnb nextjs"
LOG_DIR="$PROJECT_ROOT/logs"
MAX_LOG_LINES=300
MAX_LOG_FILES=5

echo "========================================"
echo "  Coldnb Development Restart"
echo "========================================"
echo ""

# Create log directory if it doesn't exist
mkdir -p "$LOG_DIR"

# Rotate logs: moves .4 -> .5, .3 -> .4, .2 -> .3, .1 -> .2, current -> .1
# Keeps last MAX_LOG_FILES sessions, deletes oldest
rotate_log() {
    local name=$1  # e.g. "backend" or "frontend"
    local current="$LOG_DIR/${name}.txt"

    # Nothing to rotate if no current log exists
    if [ ! -f "$current" ]; then
        return
    fi

    # Delete the oldest log
    rm -f "$LOG_DIR/${name}.${MAX_LOG_FILES}.txt"

    # Shift existing logs: .4 -> .5, .3 -> .4, etc.
    for i in $(seq $((MAX_LOG_FILES - 1)) -1 1); do
        if [ -f "$LOG_DIR/${name}.${i}.txt" ]; then
            mv "$LOG_DIR/${name}.${i}.txt" "$LOG_DIR/${name}.$((i + 1)).txt"
        fi
    done

    # Save last 300 lines of current log as .1 (previous session)
    tail -n $MAX_LOG_LINES "$current" > "$LOG_DIR/${name}.1.txt"
    rm -f "$current"
}

# Function to kill process on port
kill_port() {
    local port=$1
    local pids=$(ss -tlnp | grep ":$port " | grep -oP 'pid=\K[0-9]+' || true)
    if [ -n "$pids" ]; then
        for pid in $pids; do
            echo "  Stopping process on port $port (PID: $pid)"
            kill -9 $pid 2>/dev/null || true
        done
        sleep 1
    else
        echo "  Port $port is already free"
    fi
}

# Step 1: Stop existing servers
echo ">> Step 1: Stopping existing servers"
# Kill all next-server processes (they spawn on multiple ports)
for port in 3000 3001 3002 3003 3004 3005; do
    kill_port $port
done
kill_port 8080  # Backend
# Also kill any remaining next/node processes from our project
pkill -9 -f "next-server" 2>/dev/null || true
pkill -9 -f "next-router-worker" 2>/dev/null || true
sleep 2
echo ""

# Step 2: Save and rotate logs from previous session
echo ">> Step 2: Saving previous logs"
rotate_log "backend"
rotate_log "frontend"
echo "  Logs saved to $LOG_DIR/"
echo "  Previous session: backend.1.txt, frontend.1.txt"
echo ""

# Step 3: Clean cache
echo ">> Step 3: Cleaning cache"
echo "  Cleaning backend build artifacts..."
cd "$BACKEND_DIR"
make clean 2>/dev/null || true

echo "  Cleaning frontend .next cache..."
cd "$FRONTEND_DIR"
rm -rf .next
echo "  Cache cleaned"
echo ""

# Step 4: Rebuild backend
echo ">> Step 4: Building backend"
cd "$BACKEND_DIR"
make
echo "  Backend built"
echo ""

# Step 5: Start backend
echo ">> Step 5: Starting backend server"
cd "$BACKEND_DIR"
nohup ./build/bin/coldnb-server -c config/server.conf > "$LOG_DIR/backend.txt" 2>&1 &
BACKEND_PID=$!
echo "  Backend started (PID: $BACKEND_PID, Port: 8080)"
echo ""

# Wait for backend to be ready
echo "  Waiting for backend to respond..."
for i in {1..10}; do
    if curl -sf http://localhost:8080/api/products --max-time 2 &>/dev/null; then
        echo "  Backend is responding"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "  Backend not responding yet, but continuing..."
    fi
    sleep 1
done
echo ""

# Step 6: Start frontend
echo ">> Step 6: Starting frontend server"
cd "$FRONTEND_DIR"
nohup npm run dev > "$LOG_DIR/frontend.txt" 2>&1 &
FRONTEND_PID=$!
echo "  Frontend started (PID: $FRONTEND_PID, Port: 3000)"
echo ""

# Summary
echo "========================================"
echo "  Servers Running"
echo "========================================"
echo "Backend:  http://localhost:8080 (PID: $BACKEND_PID)"
echo "Frontend: http://localhost:3000 (PID: $FRONTEND_PID)"
echo ""
echo "Live Logs:"
echo "  tail -f $LOG_DIR/backend.txt"
echo "  tail -f $LOG_DIR/frontend.txt"
echo ""
echo "Previous Session Logs (crash investigation):"
echo "  cat $LOG_DIR/backend.1.txt"
echo "  cat $LOG_DIR/frontend.1.txt"
echo ""
echo "Stop Servers:"
echo "  /home/lucas/coldnb/scripts/dev-stop.sh"
echo "========================================"
