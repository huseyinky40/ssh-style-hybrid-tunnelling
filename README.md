# SSH-Style Hybrid Tunnelling System 🔒📡

## Overview
This project presents the implementation of a secure SSH-style tunnelling system developed in **C**. It establishes a protected communication channel between a client and a server using a hybrid cryptographic approach. The system is designed to simulate real-world network challenges, including noise and packet corruption, and ensures data integrity through advanced error recovery mechanisms.

## Key Features
* **Hybrid Encryption Architecture**:
    * **Asymmetric (RSA)**: Used for the initial secure handshake to exchange a 128-bit session key.
    * **Symmetric (AES-128 ECB)**: All subsequent data payloads, including text messages and binary files, are encrypted using AES-128 in Electronic Codebook mode.
* **Reliable Data Transfer (RDT)**: Implements a **Stop-and-Wait ARQ** (Automatic Repeat Request) protocol. The system segments data into frames and waits for positive acknowledgments (ACK) before proceeding.
* **Error Detection & Resilience**: Every frame is protected by a **CRC32 checksum**. If the receiver detects corruption, it issues a NACK (Negative Acknowledgment), triggering an automatic retransmission.
* **Network Simulation (Middlebox)**: Includes a dedicated "Proxy" application that acts as a transparent TCP relay and injects random bit-flips to test the protocol's robustness in unstable network environments.

## System Architecture
The system consists of three primary components:
1. **The Client Node**: Initiates the connection, generates the AES session key, encrypts local files/messages, and manages the transmission loop.
2. **The Server Node**: Maintains the RSA private key for the handshake, validates CRC checksums, sends ACK/NACK responses, and manages file reconstruction on the disk.
3. **The Middlebox (Proxy)**: Sits between the client and server to simulate network noise by randomly corrupting passing frames, testing the system's resilience.

## Technical Stack
* **Language**: C.
* **Networking**: TCP Socket Programming.
* **Cryptography**: RSA (Key Exchange) and AES-128 (Symmetric Encryption).
* **Integrity**: CRC32 Checksums.

## How to Build and Run
### 1. Compilation
Use the provided `Makefile` to compile the server, client, and tunnel:
```bash
make
```

### 2. Execution (In three separate terminals)
1. **Start the Server**: 
   `$ ./server`
2. **Start the Tunnel**: 
   `$ ./tunnel 12344 127.0.0.1 12345`
3. **Start the Client**: 
   `$ ./client 127.0.0.1 12344 <name>`

### 3. Client Commands
* `msg <text>`: Sends an encrypted message.
* `file <filename>`: Performs secure file transfer.
* `baglantikapa`: Closes the connection.
