#!/bin/bash
# Coldnb Development Stop Script
# Stops both backend and frontend servers (including zombie child processes)

echo "Stopping Coldnb servers..."

# Kill backend (port 8080)
BACKEND_PID=$(ss -tlnp | grep ":8080 " | grep -oP 'pid=\K[0-9]+' || true)
if [ -n "$BACKEND_PID" ]; then
    kill -9 $BACKEND_PID 2>/dev/null || true
    echo "  Backend stopped (was PID: $BACKEND_PID)"
else
    echo "  Backend not running"
fi

# Kill all next-server processes (they can spawn on ports 3000-3005)
FOUND_FRONTEND=false
for port in 3000 3001 3002 3003 3004 3005; do
    PIDS=$(ss -tlnp | grep ":$port " | grep -oP 'pid=\K[0-9]+' || true)
    if [ -n "$PIDS" ]; then
        for pid in $PIDS; do
            kill -9 $pid 2>/dev/null || true
            echo "  Frontend stopped on port $port (was PID: $pid)"
            FOUND_FRONTEND=true
        done
    fi
done

# Kill any remaining next processes
pkill -9 -f "next-server" 2>/dev/null || true
pkill -9 -f "next-router-worker" 2>/dev/null || true

if [ "$FOUND_FRONTEND" = false ]; then
    echo "  Frontend not running"
fi

echo "All servers stopped."
