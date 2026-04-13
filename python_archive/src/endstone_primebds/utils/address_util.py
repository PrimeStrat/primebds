import ipaddress
import re
import socket

def is_valid_ip(ip: str) -> bool:

    # Validate IPv4
    try:
        socket.inet_aton(ip)
        return True
    except socket.error:
        pass

    # Validate hostname (RFC 1123)
    hostname_regex = re.compile(
        r"^(?=.{1,253}$)(?!-)[A-Za-z0-9-]{1,63}(?<!-)"
        r"(?:\.(?!-)[A-Za-z0-9-]{1,63}(?<!-))*\.?$"
    )
    return bool(hostname_regex.match(ip))

def is_valid_port(port_str: str) -> bool:
    if not port_str.isdigit():
        return False
    port = int(port_str)
    return 1 <= port <= 65535

def strip_port(ip_with_port: str) -> str:
    return ip_with_port.split(":")[0] if ip_with_port else ""

def same_subnet(ip1: str, ip2: str, subnet_mask: int = 24) -> bool:
    try:
        ip1_clean = strip_port(ip1)
        ip2_clean = strip_port(ip2)
        network1 = ipaddress.IPv4Network(f"{ip1_clean}/{subnet_mask}", strict=False)
        network2 = ipaddress.IPv4Network(f"{ip2_clean}/{subnet_mask}", strict=False)
        return network1 == network2
    except Exception:
        return False



