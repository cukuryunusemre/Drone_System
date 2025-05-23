# 🚁 Emergency Drone Coordination System

This project simulates a real-time drone coordination system for emergency aid delivery using a multi-threaded server, socket-based clients, and SDL2 visualization. Drones autonomously navigate to help survivors based on proximity and availability.

---

## 🔧 Features

- 🧵 Multi-threaded server with POSIX threads
- 📡 TCP-based client-server communication
- 📍 Autonomous drone assignment based on proximity (AI logic)
- ⏱️ Timeout detection for disconnected drones
- 📊 Real-time statistics: mission count, average duration, disconnects
- 🖼️ SDL2-based graphical interface to visualize survivors and drones

---

## 📂 Project Structure

```
.
├── server.c             # Main server loop (accepts drones, starts threads)
├── client.c             # Drone client that connects and moves toward targets
├── drone_handler.c/h    # Handles incoming drone messages and mission logic
├── survivor.c/h         # Generates survivors periodically
├── view.c/h             # SDL2 real-time visualization (Phase 3)
├── list.c/h             # Thread-safe generic linked list
├── coord.h              # Coord struct used for (x, y) representation
├── protocol.h           # Message keys and buffer settings
```

---

## ⚙️ How It Works

### 🧠 AI Controller Logic
Drones are assigned to the closest unassisted survivor using Manhattan distance:
- Survivors are marked as `helped` when assigned
- If a drone disconnects, its target is re-opened for others

### 🎯 Drone Behavior
Each drone:
- Sends its position via `STATUS_UPDATE`
- Receives a mission (`ASSIGN_MISSION`) from the server
- Moves step-by-step toward the target
- Sends `MISSION_COMPLETE` on arrival

### 📺 Visualization
SDL2 draws a 40x30 grid:
- 🟥 Red squares → unassisted survivors
- 🔵 Blue circles → drones
- Screen updates every 1 second

---

## 🛠️ Compilation

### Server:
```bash
gcc server.c drone_handler.c survivor.c list.c view.c -o server -lpthread -ljson-c -lSDL2
```

### Client:
```bash
gcc client.c -o client -ljson-c
```

---

## ▶️ Run

In separate terminals:

```bash
# Start server
./server

# Start drone clients
./client D1
./client D2
./client D3
```

---

## 📊 Sample Output

**Server:**
```
🚁 Drone D1 → Survivor (10, 22) assigned
✅ Drone D1 completed mission. Duration: 5s
📊 Missions: 7 | Disconnects: 0 | Avg Time: 3.7s
```

**Client:**
```
🛰️ D1 received mission: (10, 22)
📍 D1 → (5, 12) → target: (10, 22)
✅ D1 completed mission!
```

**Visualizer:**
- SDL2 window shows moving drones and new survivors every 2 seconds.

---

## 🧠 Learning Objectives

- Implement thread-safe shared data structures
- Design and manage socket-based client-server communication
- Visualize concurrency and coordination in real time
- Apply real-world fault tolerance (e.g., drone disconnects)

---

## 📌 Dependencies

- `pthread`
- `json-c`
- `SDL2`

Install SDL2 on Ubuntu:
```bash
sudo apt install libsdl2-dev
```

---

## 🧑‍💻 Contributors

- Project Lead: [Your Name]
- System Architecture: You
- Visual Interface: You
- Socket Integration: You

---
