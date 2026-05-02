# CSE 320 — Computer Networks: TCP Congestion Control & Routing Simulation

## 1. Team Members & Roles

This project was developed collaboratively by the following team members:

| Name                 | Student ID  | Presentation & Technical Role                                                                                       |
| :------------------- | :---------- | :------------------------------------------------------------------------------------------------------------------ |
| **Kerem Düz**        | 20220808053 | **System Setup & Build:** Compilation processes, Winsock integration, and environment configuration.                |
| **Furkan Tosun**     | 20220808025 | **Algorithm Validation:** TCP Tahoe trace execution, cwnd evolution analysis, and event handling.                   |
| **Ömer Faruk Önder** | 20220808072 | **Network Simulation:** Multi-node topology demonstration, Distance Vector convergence, and live packet forwarding. |

---

## 2. Demonstration Video Structure

The project demonstration is divided into three main segments following the team roles:

1.  **Environment & Compilation (Kerem Düz):**
    - Demonstration of `gcc` compilation with `-lws2_32`.
    - Initial node startup and configuration loading (`A.conf`).
2.  **TCP Tahoe Logic Validation (Furkan Tosun):**
    - Execution of `duplicate_ack_test.txt` to show Triple Duplicate ACK handling.
    - Execution of `timeout_test.txt` to show Timeout-based Tahoe Reset.
    - Step-by-step analysis of `cwnd` and `ssthresh` transitions.
3.  **Distributed Network & Routing (Ömer Faruk Önder):**
    - Simultaneous execution of Nodes A through F.
    - Displaying routing table convergence (`table` command).
    - Verifying the optimal path for `send E to F`.

---

## 3. Algorithm Assignment

As per the assignment guidelines, our algorithm was determined using the formula:
`([student1 id + student2 id + student3 id] mod 3)`

**Calculation:**
`(20220808072 + 20220808053 + 20220808025) % 3 = 0`

**Result:** `0` -> **TCP Tahoe**

---

## 4. Project Overview

This project simulates a multi-node network where each node acts as a router (Distance Vector) and a TCP sender/receiver. It demonstrates the interaction between network-layer routing and transport-layer congestion control.

### Key Features:

- **Distance Vector Routing:** Nodes exchange routing vectors periodically to converge on the shortest path (Bellman-Ford algorithm).
- **TCP Tahoe Implementation:**
  - **Slow Start:** `cwnd` grows exponentially (adds 1.0 per ACK).
  - **Congestion Avoidance:** `cwnd` grows linearly (adds 1/cwnd per ACK) after reaching `ssthresh`.
  - **Fast Retransmit:** Detection of 3 duplicate ACKs triggers a retransmit.
  - **Loss Handling:** On both Timeout and Triple Duplicate ACKs, Tahoe resets `cwnd` to 1.0 MSS and adjusts `ssthresh` to `cwnd/2`.
- **Trace Mode:** Step-by-step visualization of congestion window evolution using event files.
- **Live Mode:** Real-time socket communication between nodes in a distributed environment.

---

## 5. System Architecture

The implementation is divided into two core modules:

1.  **Network Engine (`node.c`):** Handles UDP socket management, neighbor discovery, and Distance Vector updates.
2.  **Congestion Control Logic:** A state-driven implementation of the TCP Tahoe algorithm (Slow Start vs. Congestion Avoidance).

### Topology Information:

The simulation uses a six-node topology (A-F) with the following link costs (A as reference):

- **A to B:** 4 | **A to C:** 7 | **A to F:** 5
- **A to D:** Indirect path via B (Cost: 12) is preferred over direct path (Cost: 13).

---

## 6. Compilation & Execution

### Prerequisites

- GCC Compiler
- Windows Environment (Uses Winsock2)

### Compilation

```powershell
gcc node.c -o node.exe -lws2_32
```

### Running the Trace (Algorithm Validation)

To verify the TCP Tahoe behavior against specific packet loss scenarios:

```powershell
# Test Triple Duplicate ACKs
.\node.exe A.conf events/duplicate_ack_test.txt

# Test Timeout Scenarios
.\node.exe A.conf events/timeout_test.txt
```

### Running the Multi-Node Network

Open six terminals to simulate the full topology:

```powershell
.\node.exe A.conf
.\node.exe B.conf
# ... repeat for C, D, E, F
```

- **Commands:**
  - `table`: Displays the current routing table and shortest paths.
  - `send <Dest> <Message>`: Routes a data packet through the network.
  - `exit`: Safely closes the node.

---

## 7. Experimental Analysis

Our implementation demonstrates the following Tahoe characteristics:

1.  **Recovery Speed:** On any loss, Tahoe enters Slow Start immediately to ensure network stability.
2.  **Throughput:** The linear increase in Congestion Avoidance ensures fair bandwidth utilization once the pipeline is full.
3.  **Correctness:** Routing tables successfully adapt to the shortest path, ensuring packets avoid high-cost direct links when better alternatives exist.
