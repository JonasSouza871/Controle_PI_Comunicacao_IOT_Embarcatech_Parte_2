#ifndef _LWIPOPTS_EXAMPLE_COMMONH_H
#define _LWIPOPTS_EXAMPLE_COMMONH_H

//Configurações comuns para exemplos do Pico W com a biblioteca lwIP

//Configurações do sistema
#ifndef NO_SYS
#define NO_SYS                      1  //Desativa o uso de sistema operacional (bare-metal)
#endif
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0  //Desativa a API de sockets
#endif

//Configurações de memória
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1  //Usa alocação de memória da libc em modo polling
#else
#define MEM_LIBC_MALLOC             0  //Desativa alocação da libc em modos não-polling
#endif
#define MEM_ALIGNMENT               4   //Alinhamento de memória em 4 bytes
#define MEM_SIZE                    4000 //Tamanho total do heap em bytes
#define MEMP_NUM_TCP_SEG            32  //Número de segmentos TCP na memória
#define MEMP_NUM_ARP_QUEUE          10  //Tamanho da fila ARP
#define PBUF_POOL_SIZE              24  //Número de buffers no pool de pacotes

//Configurações de protocolos de rede
#define LWIP_ARP                    1   //Habilita o protocolo ARP
#define LWIP_ETHERNET               1   //Habilita suporte a Ethernet
#define LWIP_ICMP                   1   //Habilita o protocolo ICMP (ping)
#define LWIP_RAW                    1   //Habilita a API raw para pacotes
#define LWIP_IPV4                   1   //Habilita suporte a IPv4
#define LWIP_DHCP                   1   //Habilita o cliente DHCP
#define LWIP_DNS                    1   //Habilita suporte a DNS
#define DHCP_DOES_ARP_CHECK         0   //Desativa verificação ARP pelo DHCP
#define LWIP_DHCP_DOES_ACD_CHECK    0   //Desativa verificação de conflito de endereço

//Configurações de TCP
#define LWIP_TCP                    1   //Habilita o protocolo TCP
#define TCP_MSS                     1460 //Tamanho máximo do segmento TCP
#define TCP_WND                     (8 * TCP_MSS) //Janela de recepção TCP
#define TCP_SND_BUF                 (8 * TCP_MSS) //Buffer de envio TCP
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS)) //Tamanho da fila de envio
#define LWIP_TCP_KEEPALIVE          1   //Habilita keepalive para conexões TCP

//Configurações de UDP
#define LWIP_UDP                    1   //Habilita o protocolo UDP

//Configurações da interface de rede
#define LWIP_NETIF_STATUS_CALLBACK  1   //Habilita callback de status da interface
#define LWIP_NETIF_LINK_CALLBACK    1   //Habilita callback de estado do link
#define LWIP_NETIF_HOSTNAME         1   //Habilita configuração de hostname
#define LWIP_NETIF_TX_SINGLE_PBUF   1   //Usa um único buffer para transmissão
#define LWIP_NETCONN                0   //Desativa a API netconn

//Configurações de estatísticas e depuração
#define MEM_STATS                   0   //Desativa estatísticas de memória
#define SYS_STATS                   0   //Desativa estatísticas do sistema
#define MEMP_STATS                  0   //Desativa estatísticas de pools de memória
#define LINK_STATS                  0   //Desativa estatísticas do link
#ifndef NDEBUG
#define LWIP_DEBUG                  1   //Habilita modo de depuração
#define LWIP_STATS                  1   //Habilita coleta de estatísticas
#define LWIP_STATS_DISPLAY          1   //Habilita exibição de estatísticas
#endif

//Configurações de checksum e algoritmos
#define LWIP_CHKSUM_ALGORITHM       3   //Algoritmo de checksum otimizado

//Níveis de depuração por módulo (todos desativados por padrão)
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* _LWIPOPTS_EXAMPLE_COMMONH_H */