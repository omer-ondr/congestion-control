# CSE 320 — TCP Congestion Control Project (TCP Tahoe)

## 1. Group Information
- **Ömer Faruk Önder:** 20220808072
- **Kerem Düz:** 20220808053
- **Furkan Tosun:** 20220808025

**Algorithm Assignment Calculation:**
(20220808072 + 20220808053 + 20220808025) % 3 = 60,662,424,150 % 3 = 0
**Assigned Algorithm:** 0 -> **TCP Tahoe**

---

## 2. Implementation Overview
This project implements a Node-style network with TCP Tahoe congestion control. It consists of two main layers:
1.  **Network Layer:** Implements a Distance Vector routing protocol. Nodes discover the network topology and find the shortest paths (e.g., A to D goes through B because cost 4+8=12 < 13).
2.  **Transport Layer (TCP Tahoe):** Simulates Tahoe's congestion window (cwnd) behavior, including:
    - **Slow Start:** Exponential growth of cwnd.
    - **Congestion Avoidance:** Additive increase (1/cwnd) after ssthresh is reached.
    - **Fast Retransmit:** Triggered by 3 duplicate ACKs.
    - **Tahoe Reset:** On packet loss (Timeout or 3 DUPACKs), cwnd is reset to 1.0 MSS and ssthresh is set to cwnd/2.

---

## 3. System Requirements & Topology
The system models the following six-node topology:
- **Nodes:** A, B, C, D, E, F
- **Ports:** A:5001, B:5002, C:5003, D:5004, E:5005, F:5006
- **Link Costs (from Node A):** 
    - A-B: 4, A-C: 7, A-F: 5, A-D: 13 (Direct)
    - Optimal Path A -> D: A -> B -> D (Cost: 12)

---

## 4. Compilation and Execution

### Compilation
Use GCC with the Winsock library (on Windows):
```powershell
gcc node.c -o node.exe -lws2_32
```

### Running the Simulation (Trace Mode)
To demonstrate Tahoe's congestion window evolution step-by-step:
```powershell
.\node.exe A.conf events/duplicate_ack_test.txt
.\node.exe A.conf events/timeout_test.txt
```

### Running the Live Network (Multi-Terminal)
Open 6 separate terminals and run each node:
```powershell
.\node.exe A.conf
.\node.exe B.conf
.\node.exe C.conf
.\node.exe D.conf
.\node.exe E.conf
.\node.exe F.conf
```
- Type `table` in any console to view the routing table.
- Type `send <Dest> <Msg>` to simulate packet forwarding.

---

## 5. Input and Output Expectations
The program prints the evolution of `cwnd`, `ssthresh`, and the current state (Slow Start/Congestion Avoidance).

**Triple Duplicate ACK Scenario:**
- When 3 DUPACKs are detected, the output will show:
  `[Round X] | DUPACK | ACK=3 | cwnd=1.00 | ssthresh=X.XX | Slow Start | 3 DUPACKs - Tahoe Reset to 1`

**Timeout Scenario:**
- When a TIMEOUT is detected, the output will show:
  `[Round X] | TIMEOUT | ACK=0 | cwnd=1.00 | ssthresh=X.XX | Slow Start | Timeout - Tahoe Reset to 1`

---

