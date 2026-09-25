import os
import re
import socket
import struct
import time
import random

def limpiar3(texto):
    return re.sub(r"[§&].", "", texto)

RAKNET_MAGIC = b"\x00\xff\xff\x00\xfe\xfe\xfe\xfe\xfd\xfd\xfd\xfd\x12\x34\x56\x78"

def query(address):
    ip = address
    port = 19132

    if ":" in address:
        parts = address.split(":", 1)
        ip = parts[0]
        try:
            port = int(parts[1])
        except ValueError:
            print("puerto invalido")
            return

    timeout = 5
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(timeout)

    try:
        os.system("clear")
        print(f"    send packet --> {ip}:{port} ")

        client_guid = random.getrandbits(64)
        send_time = int(time.time() * 1000) & 0xFFFFFFFFFFFFFFFF

        pkt = (
            b"\x01"
            + struct.pack(">Q", send_time)
            + RAKNET_MAGIC
            + struct.pack(">Q", client_guid)
        )
        s.sendto(pkt, (ip, port))
        data, addr = s.recvfrom(4096)

        if data[0] != 0x1c:
            print("respuesta invalida (no es unconnected pong)")
            return

        offset = 1
        server_time = struct.unpack(">Q", data[offset:offset+8])[0]
        offset += 8
        server_guid = struct.unpack(">Q", data[offset:offset+8])[0]
        offset += 8
        magic = data[offset:offset+16]
        offset += 16
        str_len = struct.unpack(">H", data[offset:offset+2])[0]
        offset += 2
        server_id_str = data[offset:offset+str_len].decode(errors="ignore")

        campos = server_id_str.split(";")

        def get(i, default="none"):
            return campos[i] if i < len(campos) else default

        edition = get(0)
        motd = limpiar3(get(1))
        protocol = get(2)
        version = get(3)
        num_players = get(4, "0")
        max_players = get(5, "0")
        server_id = get(6)
        submotd = limpiar3(get(7, ""))
        gamemode = get(8)
        gamemode_num = get(9)
        port_v4 = get(10)
        port_v6 = get(11)

        print(f"Edition: {edition}")
        print(f"MOTD: {motd}")
        print(f"Sub-MOTD: {submotd}")
        print(f"Protocolo: {protocol}")
        print(f"Version: {version}")
        print(f"Jugadores: {num_players}/{max_players}")
        print(f"Gamemode: {gamemode} ({gamemode_num})")
        print(f"Server GUID: {server_guid}")
        print(f"Server ID: {server_id}")
        print(f"Puerto v4/v6: {port_v4}/{port_v6}")

    except socket.timeout:
        print(f"{ip}:{port} --> timeout")
    finally:
        s.close()

print("Se debe usar como IP:PORT (1.1.1.1:19132)")
data = input("> ")
if data:
    query(data)
