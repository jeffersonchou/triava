#!/system/bin/sh

#Flush iptables
iptables -t nat -F TRIAVA
iptables -t nat -F OUTPUT
#iptables -t filter -F OUTPUT
iptables -t nat -X TRIAVA

#Configure iptables
iptables -t nat -N TRIAVA
iptables -t nat -A TRIAVA -m owner --uid-owner root -j RETURN
iptables -t nat -A TRIAVA -p 6 -m tcp -d 127.0.0.0/24 -j RETURN
iptables -t nat -A TRIAVA -p 6 -j REDIRECT --to-ports 12345
iptables -t nat -A OUTPUT -p 6 -j TRIAVA


#Run Triava
/data/Triava/Triava --no-daemon /data/Triava/etc/config &

#Flush iptables
#iptables -t nat -F TRIAVA
#iptables -t nat -F OUTPUT
#iptables -t filter -F OUTPUT
#iptables -t nat -X TRIAVA

