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
				# Byteplus Pte. Ltd. - https://www.byteplus.com
				$AS150436,
				# VNPT Corp - http://www.vnpt.vn
				$AS45899,
				# Tencent - http://www.tencent.com
				$AS132203,
				# Telkom SA Ltd. - http://www.telkomsa.net/
				$AS37457,
				# SUPER CELL NETWORK FOR INTERNET SERVICES LTD - https://supercell.iq/
				$AS201749,
				# Hilal Al-Rafidain for Computer and Internet Services ... - https://www.hrins.net/ftth
				$AS209273,
				# Pacifico Cable SPA. - http://www.tumundo.cl
				$AS27901,
				# OneCable Network LLC - https://www.onecablenetwork.com
				$AS401560,
				# "Uzbektelekom" Joint Stock Company
				$AS8193,
				# Sibirskie Seti Ltd. - http://sibset.ru
				$AS34757,
				# Cobranet Limited - http://www.cobranet.ng
				$AS37480,
				# TIGO Paraguay - https://www.tigo.com.py
				$AS23201,
				# Vodafone Qatar P.Q.S.C - https://www.vodafone.qa/
				$AS48728,
				# PT Telkom Indonesia Tbk - http://www.telin.net
				$AS7713,
				# Jordan Telecommunications PSC - http://www.orange.jo
				$AS8697,
				# PT Cyberindo Aditama - https://www.cbn.id
				$AS135478,
				# Wataniya Telecom Algerie - http://www.ooredoo.dz
				$AS33779,
				# China Unicom Guangdong Province Shenzhen Network
				$AS17623,
				# METRONET
				$AS262262,
				# Cyber Info Provedor de Acesso LTDA ME - http://www.cyberinfo.net.br
				$AS53246,
				# Maroc Telecom - https://www.iam.ma/
				$AS6713,
				# UFINET PANAMA S.A. - http://www.ufinet.com/
				$AS52468,
				# COLOMBIA TELECOMUNICACIONES S.A. ESP BIC - https://www.movistar.com.co/
				$AS3816,
				# Wana Corporate - http://www.inwi.ma
				$AS36884,
				# Dialnet de Colombia S.A. E.S.P. - http://www.dialnet.net
				$AS27837,
				# China Unicom Backbone - http://www.10010.com
				$AS4837,
				# NETFIBRA TELECOMUNICACOES LTDA - ME - http://www.netfibratelecom.com.br
				$AS267106,
				# China Telecom Hangzhou IDC Network
				$AS58461,
				# "Bulgartel" AD - https://bulgartel.bg/
				$AS44814,
				# PJSC MegaFon
				$AS24866,
				# Korea Telecom - https://www.kt.com/
				$AS4766,
				# Hostinger International Limited - https://www.hostinger.com/
				$AS47583,
				# Telcorp Latam - https://telcorplatam.com/
				$AS269918,
				# Hangzhou Alibaba Advertising Co.,Ltd. - http://alibabagroup.com/
				$AS37963,
				# Kcell JSC - http://www.kcell.kz
				$AS29355,
				# Telefonica de Argentina - https://www.movistar.com.ar/
				$AS22927,
				# TELEFONICA CHILE S.A - http://www.movistar.cl
				$AS7418,
				# TELEFONICA BRASIL S.A- http://www.telefonica.com.br
				$AS27699,
				# V.tal (fka Telemar) - http://www.vtal.com
				$AS7738,
				# Bharti Airtel Ltd. AS for GPRS Service
				$AS45609,
				# V.tal - http://www.vtal.com
				$AS8167,
				# Orange Maroc - http://www.orange.ma
				$AS36925,
				# China Telecom Backbone - https://www.189.cn/
				$AS4134,
				# YANDEX LLC - https://www.ya.ru
				$AS13238,
				# Edge Technology Plus (Yandex) - https://ya.ru
				$AS208398,
				# China Telecom Beijing IDC Network
				$AS23724,
				# China Telecom Shanghai IDC Network
				$AS4811,
				# Hulum Almustakbal Company
				$AS203214,
				# TerraNet sal - https://www.terra.net.lb
				$AS39010,
				# Virtual Systems LLC - https://vsys.host
				$AS30860,
				# Piranha Systems - https://myip.co.kr
				$AS38701,
				# Google Cloud Platform - https://cloud.google.com
				$AS396982,
				# Tencent Cloud - https://cloud.tencent.com
				$AS45090,
				# Netinternet Bilisim Teknolojileri AS - https://netinternet.tr
				$AS51559,
				# Beijing Volcano Engine Technology Co., Ltd. - https://www.volcengine.com
				$AS137718,
				# PT Reconet Semesta Indonesia - https://reconet.co.id
				$AS150936,
				# LG DACOM Corporation - http://www.lguplus.com
				$AS3786,
				# Unwired Communications Limited - http://www.unwired.co.ke
				$AS328490,
				# Techno Asia InfoTech Pvt. ltd
				$AS135037,
				# Triple T Broadband Public Company Limited - http://www.3bb.co.th
				$AS45758,
				# Bharti Airtel Ltd., Telemedia Services - https://airtel.in
				$AS24560,
				# Vietnam P&T - https://vnpt.com.vn
				$AS135905,
				# Information and Communication Technology Agency of Srilanka
				$AS132124,
				# TECHOFF SRV LIMITED - https://dmzhost.co/
				$AS48090,
				# Psychz Networks - https://www.psychz.net
				$AS40676,
				# China Telecom Jiangsu Province IDC Network
				$AS23650,
				# CS LOXINFO Public Company Limited. - https://www.csloxinfo.com/idc-solutions
				$AS9891,
				# IONOS SE - http://www.ionos.com
				$AS8560,
				# SK Broadband Co Ltd - https://www.bworld.co.kr
				$AS9318,
				# DigitalOcean LLC - https://www.digitalocean.com
				$AS14061,
				# TOTAL PLAY TELECOMUNICACIONES SA DE CV- http://www.totalplay.com.mx
				$AS22884,
				# FDCservers.net - https://www.fdcservers.net
				$AS30058,
				# China Telecom Wuxi IDC Network
				$AS138950,
				# China Unicom - http://www.chinaunicom.com
				$AS138421,
				# Beijing Baidu Netcom Science and Technology Co., Ltd. - http://www.baidu.com
				$AS38365,
				# CHINANET Guangdong province network
				$AS58466,
				# CHINANET Nanjing Jishan IDC network
				$AS134756,
				# CHINA TELECOM
				$AS140903,
				# China Unicom Beijing Province Network
				$AS4808,
				# China Unicom Industrial Internet Backbone
				$AS9929,
				# GoDaddy.com, LLC - https://godaddy.com
				$AS398101,
				# Zong (CMPak Limited) - https://www.zong.com.pk
				$AS59257,
				# Kompeatelecom Ltd.
				$AS25299,
				# Corporación Telemic C.A.(Inter) - https://inter.com.ve
				$AS21826,
				# Tunisia BackBone AS - http://www.tunisietelecom.tn
				$AS2609,
				# ENTEL CHILE S.A. - http://www.entel.cl
				$AS27651,
				# Bharti Airtel Ltd. - https://www.airtel.in
				$AS9498,
				# The Communications Authority of Thailand (CAT) - https://www.ntplc.co.th
				$AS4651,
				# OVH SAS - https://ovhcloud.com
				$AS16276,
			}
		}
		counter accepted_connections {}
		counter dropped_connections {}
		counter rejected_asn {}
		counter dropped_ipv4 {}
		counter accepted_webclients {}
		counter dropped_webclients {}
		counter accepted_smtpclients {}
		counter dropped_smtpclients {}
		counter accepted_sshclients {}
		counter dropped_sshclients {}
		counter accepted_mailclients {}
		counter dropped_mailclients {}
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
			ct state established,related counter name "accepted_connections" accept
			ct state invalid counter name "dropped_connections" drop
			# Reject blacklisted ASN
			ct state new ct mark set ip saddr map @dbip-ipv4-asn
			ct state new ct mark set ip6 saddr map @dbip-ipv6-asn
			ct mark @asn_blocklist counter name "rejected_asn" reject
			# Drop blacklisted web clients
			tcp dport { 80, 443 } ip saddr @ip4_blocklist\
				counter name "dropped_ipv4" drop
			# Drop web clients having exceeded rate limiting
			tcp dport { 80, 443 } ip saddr @http_blocklist update @http_blocklist {\
				ip saddr timeout 2h\
			} counter name "dropped_webclients" drop
			# Drop mail transfer agents having exceeded rate limiting
			tcp dport { 25 } ip saddr @mta_blocklist update @mta_blocklist {\
				ip saddr timeout 2h\
			} counter name "dropped_smtpclients" drop
			# Drop ssh clients having exceeded rate limiting
			tcp dport { 22 } ip saddr @ssh_blocklist update @ssh_blocklist {\
				ip saddr timeout 2h\
			} counter name "dropped_sshclients" drop
			# Drop mail user agents having exceeded rate limiting
			tcp dport { 465, 993 } ip saddr @mua_blocklist update @mua_blocklist {\
				ip saddr timeout 2h\
			} counter name "dropped_mailclients" drop
			# Accept web clients in the country blocklist applying hard rate limiting
			meta mark set ip saddr map @dbip-ipv4-country
			meta mark set ip6 saddr map @dbip-ipv6-country
			meta mark @country_blocklist tcp dport { 80, 443 }\
				ct state new update @http_limits_hard {\
					ip saddr limit rate 1/hour burst 25 packets\
				} counter name "accepted_webclients" accept
			# Accept web clients not in the country blocklist applying soft rate limiting
			meta mark != @country_blocklist tcp dport { 80, 443 }\
				ct state new update @http_limits_soft {\
					ip saddr limit rate 4/minute burst 10 packets\
				} counter name "accepted_webclients" accept
			# Drop web clients exceeding rate limiting
			tcp dport { 80, 443 } ct state new update @http_blocklist {\
				ip saddr\
			} counter name "dropped_webclients" drop
			# Accept mail transfer agents applying rate limiting
			tcp dport { 25 } ct state new update @mta_limits {\
				ip saddr limit rate 1/minute burst 10 packets\
			} counter name "accepted_smtpclients" accept
			# Drop mail transfer agents exceeding rate limiting
			tcp dport { 25 } ct state new update @mta_blocklist {\
				ip saddr\
			} counter name "dropped_smtpclients" drop
			# Drop ssh clients not from germany
			meta mark != { $DE } tcp dport { 22 } update @ssh_blocklist {\
				ip saddr timeout 1h\
			} counter name "dropped_sshclients" drop
			# Accept ssh clients applying rate limiting
			tcp dport { 22 } ct state new update @ssh_limits {\
				ip saddr limit rate 1/minute burst 2 packets\
			} counter name "accepted_sshclients" accept
			# Drop ssh clients exceeding rate limiting
			tcp dport { 22 } ct state new update @ssh_blocklist {\
				ip saddr\
			} counter name "dropped_sshclients" drop
			# Accept mail user agents applying rate limiting
			tcp dport { 465, 993 } ct state new update @mua_limits {\
				ip saddr limit rate 1/minute burst 10 packets\
			} counter name "accepted_mailclients" accept
			# Drop mail user agents exceeding rate limiting
			tcp dport { 465, 993 } ct state new update @mua_blocklist {\
				ip saddr\
			} counter name "dropped_mailclients" drop
		}
		chain forward {
			type filter hook forward priority filter;
		}
		chain output {
			type filter hook output priority filter;
		}
	}

	0x02$ nft list counters
	0x02$ nft list set inet filter http_blocklist
	0x02$ nft list set iner filter ssh_blocklist
	0x02$ nft list set iner filter mta_blocklist
	0x02$ nft list set iner filter mua_blocklist
	0x02$ nft list set inet filter http_limits_hard
	0x02$ nft list set inet filter http_limits_soft
	0x02$ nft list set inet filter ssh_limits
	0x02$ nft list set inet filter mta_limits
	0x02$ nft list set inet filter mua_limits
	0x02$ whois -h whois.cymru.com -v 2.58.100.1
	0x02$ lynx https//bgp.tools/as/3320

