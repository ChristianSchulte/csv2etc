	Usage: csv2etc command [-q] [-i input-file]
		dbip-asn
			nftables [-n name] [-o out-dir]
		dbip-country
			nftables [-n name] [-o out-dir]

	0x02$ wget -q https://download.db-ip.com/free/dbip-country-lite-$(date +%Y-%m).csv.gz
	0x02$ wget -q https://download.db-ip.com/free/dbip-asn-lite-$(date +%Y-%m).csv.gz

	0x02$ gzip -cd dbip-asn-lite-$(date +%Y-%m).csv.gz | csv2etc dbip-asn nftables -o /etc/nftables
	/etc/nftables/dbip-asn-def.nft
	/etc/nftables/dbip-asn-ipv4.nft
	/etc/nftables/dbip-asn-ipv6.nft

	0x02$ gzip -cd dbip-country-lite-$(date +%Y-%m).csv.gz | csv2etc dbip-country nftables -o /etc/nftables
	/etc/nftables/dbip-country-def.nft
	/etc/nftables/dbip-country-ipv4.nft
	/etc/nftables/dbip-country-ipv6.nft

	0x02$ cat /etc/nftables.conf
	#!/usr/sbin/nft -f

	flush ruleset

	table inet filter {
		set http_blocklist {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			timeout 1h
		}
		set http_limits_hard {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			# 25 burst / 1 rate = 25 plus 15s headroom
			timeout 25h15s
		}
		set http_limits_soft {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			# 10 burst / 4 rate = 2.5 plus 15s headroom
			timeout 165s
		}
		set mta_blocklist {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			timeout 1h
		}
		set mta_limits {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			# 10 burst / 1 rate = 10 plus 15s headroom
			timeout 10m15s
		}
		set ssh_blocklist {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			timeout 1h
		}
		set ssh_limits {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			# 2 burst / 1 rate = 2 plus 15s headroom
			timeout 2m15s
		}
		set mua_blocklist {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			timeout 1h
		}
		set mua_limits {
			type ipv4_addr
			flags dynamic, timeout
			size 524288
			# 10 burst / 1 rate = 10 plus 15s headroom
			timeout 10m15s
		}
		set ip4_blocklist {
			type ipv4_addr
			flags interval
			auto-merge
		}
		include "/etc/nftables/dbip-asn-def.nft"
		include "/etc/nftables/dbip-country-def.nft"
		include "/etc/nftables/dbip-asn-ipv4.nft"
		include "/etc/nftables/dbip-asn-ipv6.nft"
		include "/etc/nftables/dbip-country-ipv4.nft"
		include "/etc/nftables/dbip-country-ipv6.nft"
		set country_blocklist {
			type mark
			elements = {
				$CN, $RU, $KP, $VN, $IR, $BR, $ID, $TR, $UA, $ZA, $SG,
				$BD, $HK, $UZ, $HN, $AR, $PY, $NP, $EC, $IQ, $IN, $AL,
				$PH, $LB, $CG, $PK, $MY, $BW, $RO, $EG, $TH, $TN, $SA,
				$DZ, $MX, $AO, $CL, $KH, $CO, $KE, $JO, $MA, $VE, $MU,
				$KG, $TG, $KZ, $BO, $PE, $SN, $LT, $BF, $SV, $NG, $MY,
				$AE, $AM, $MM, $QA, $MQ
			}
		}
		set asn_blocklist {
			type mark
			elements= {
				# Alibaba (US) Technology Co., Ltd. - http://alibabagroup.com/
				$AS45102,
				# SEMrush CY LTD - https://www.semrush.com
				$AS209366,
			}
		}
		chain input {
			type filter hook input priority filter; policy drop;
			# Accept loopback
			iif lo accept
			# Accept ICMP
			ip protocol icmp accept
			ip6 nexthdr icmpv6 accept
			# Track connection states
			#  established: Packets that belong to an already established
			#               connection
			#  related: Packets related to an existing connection
			#  invalid: Packets that are invalid according to connection
			#           tracking
			ct state established,related accept
			ct state invalid counter drop
			# Drop blacklisted ASN
			ct state new ct mark set ip saddr map @dbip-ipv4-asn
			ct state new ct mark set ip6 saddr map @dbip-ipv6-asn
			ct mark @asn_blocklist counter drop
			# Drop blacklisted web clients
			tcp dport { 80, 443 } ip saddr @ip4_blocklist counter drop
			# Drop web clients having exceeded rate limiting
			tcp dport { 80, 443 } ip saddr @http_blocklist update @http_blocklist {\
				ip saddr timeout 2h\
			} counter drop
			# Drop mail transfer agents having exceeded rate limiting
			tcp dport { 25 } ip saddr @mta_blocklist update @mta_blocklist {\
				ip saddr timeout 2h\
			} counter drop
			# Drop ssh clients having exceeded rate limiting
			tcp dport { 22 } ip saddr @ssh_blocklist update @ssh_blocklist {\
				ip saddr timeout 2h\
			} counter drop
			# Drop mail user agents having exceeded rate limiting
			tcp dport { 465, 993 } ip saddr @mua_blocklist update @mua_blocklist {\
				ip saddr timeout 2h\
			} counter drop
			# Accept web clients in the country blocklist applying hard rate limiting
			meta mark set ip saddr map @dbip-ipv4-country
			meta mark set ip6 saddr map @dbip-ipv6-country
			meta mark @country_blocklist tcp dport { 80, 443 }\
				ct state new update @http_limits_hard {\
					ip saddr limit rate 1/hour burst 25 packets\
				} accept
			# Accept web clients not in the country blocklist applying soft rate limiting
			meta mark != @country_blocklist tcp dport { 80, 443 }\
				ct state new update @http_limits_soft {\
					ip saddr limit rate 4/minute burst 10 packets\
				} accept
			# Drop web clients exceeding rate limiting
			tcp dport { 80, 443 } ct state new update @http_blocklist {\
				ip saddr\
			} counter drop
			# Accept mail transfer agents applying rate limiting
			tcp dport { 25 } ct state new update @mta_limits {\
				ip saddr limit rate 1/minute burst 10 packets\
			} accept
			# Drop mail transfer agents exceeding rate limiting
			tcp dport { 25 } ct state new update @mta_blocklist {\
				ip saddr\
			} counter drop
			# Drop ssh clients not from germany
			meta mark != { $DE } tcp dport { 22 } update @ssh_blocklist {\
				ip saddr timeout 1h\
			} counter drop
			# Accept ssh clients applying rate limiting
			tcp dport { 22 } ct state new update @ssh_limits {\
				ip saddr limit rate 1/minute burst 2 packets\
			} accept
			# Drop ssh clients exceeding rate limiting
			tcp dport { 22 } ct state new update @ssh_blocklist {\
				ip saddr\
			} counter drop
			# Accept mail user agents applying rate limiting
			tcp dport { 465, 993 } ct state new update @mua_limits {\
				ip saddr limit rate 1/minute burst 10 packets\
			} accept
			# Drop mail user agents exceeding rate limiting
			tcp dport { 465, 993 } ct state new update @mua_blocklist {\
				ip saddr\
			} counter drop
		}
		chain forward {
			type filter hook forward priority filter;
		}
		chain output {
			type filter hook output priority filter;
		}
	}
