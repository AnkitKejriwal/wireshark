/* THIS FILE IS AUTOMATICALLY GENERATED, DO NOT MODIFY!!! */
const char *capture_filters_part[] = {
"Filtering packets while capturing \n"
"--------------------------------- \n"
"Capture Filters are used to filter out uninteresting packets already at capture time. This is done to reduce the size of the resulting capture (file) and is especially useful on high traffic networks or for long term capturing.   \n"
" \n"
"Ethereal uses the pcap (libpcap/WinPcap) filter language for capture filters. This language is explained in the tcpdump man page (http://www.tcpdump.org). \n"
" \n"
"Note: This capture filter language is different from the one used for the Ethereal display filters! \n"
" \n"
"------------------------------------------------- \n"
" \n"
"Some common examples: \n"
"--------------------- \n"
"Example Ethernet: capture all traffic to and from the Ethernet address 08.00.08.15.ca.fe \n"
" \n"
"ether 08.00.08.15.ca.fe \n"
" \n"
"Example IP: capture all traffic to and from the IP address 192.168.0.10 \n"
" \n"
"host 192.168.0.10 \n"
" \n"
"Example TCP: capture all traffic to and from the TCP port 80 (http) of all machines \n"
" \n"
"tcp port 80 \n"
" \n"
"Examples combined: capture all traffic to and from 192.168.0.10 except http \n"
" \n"
"host 192.168.0.10 and not tcp port 80 \n"
" \n"
"Beware: if you capture TCP/IP traffic with the primitives \"host\" or \"port\", you will not see the ARP traffic belonging to it! \n"
" \n"
"------------------------------------------------- \n"
" \n"
"Capture Filter Syntax: \n"
"---------------------- \n"
"The following is a short description of the capture filter language syntax. For a further reference, have a look at: http://www.tcpdump.org/tcpdump_man.html \n"
" \n"
"A capture filter takes the form of a series of primitive expressions, connected by conjunctions (and/or) and optionally preceeded by not: \n"
" \n"
"[not] primitive [and|or [not] primitive ...] \n"
" \n"
"A primitive is simply one of the following: \n"
" \n"
"[src|dst] host <host> \n"
" \n"
"This primitive allows you to filter on a host IP address or name. You can optionally preceed the primitive with the keyword src|dst to specify that you are only interested in source or destination addresses. If these are not present, packets where the specified address appears as either the source or the destination address will be selected. \n"
" \n"
"ether [src|dst] host <ehost> \n"
" \n"
"This primitive allows you to filter on Ethernet host addresses. You can optionally includethe keyword src|dst between the keywords ether and host to specify that you are only interested in source or destination addresses. If these are not present, packets where the specified address appears in either the source or destination address will be selected. \n"
" \n"
"gateway host <host> \n"
" \n"
"This primitive allows you to filter on packets that used host as a gateway. That is, where the Ethernet source or destination was host but neither the source nor destination IP address was host. \n"
" \n"
"[src|dst] net <net> [{mask <mask>}|{len <len>}] \n"
" \n"
"This primitive allows you to filter on network numbers. You can optionally preceed this primitive with the keyword src|dst to specify that you are only interested in a source or destination network. If neither of these are present, packets will be selected that have the specified network in either the source or destination address. In addition, you can specify either the netmask or the CIDR prefix for the network if they are different from your own. \n"
" \n"
"[tcp|udp] [src|dst] port <port> \n"
" \n"
"This primitive allows you to filter on TCP and UDP port numbers. You can optionally preceed this primitive with the keywords src|dst and tcp|udp which allow you to specify that you are only interested in source or destination ports and TCP or UDP packets respectively. The keywords tcp|udp must appear before src|dst. \n"
"If these are not specified, packets will be selected for both the TCP and UDP protocols and when the specified address appears in either the source or destination port field. \n"
" \n"
"less|greater <length> \n"
" \n"
"This primitive allows you to filter on packets whose length was less than or equal to the specified length, or greater than or equal to the specified length, respectively. \n"
" \n"
"ip|ether proto <protocol> \n"
" \n"
"This primitive allows you to filter on the specified protocol at either the Ethernet layer or the IP layer. \n"
" \n"
"ether|ip broadcast|multicast \n"
" \n"
"This primitive allows you to filter on either Ethernet or IP broadcasts or multicasts. \n"
" \n"
"<expr> relop <expr> \n"
" \n"
"This primitive allows you to create complex filter expressions that select bytes or ranges of bytes in packets. Please see the tcpdump man pages for more details. \n"
};
#define CAPTURE_FILTERS_PARTS 1
#define CAPTURE_FILTERS_SIZE 4397
