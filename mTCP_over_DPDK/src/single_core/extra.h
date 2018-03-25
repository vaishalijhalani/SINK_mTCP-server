#define _LARGEFILE64_SOURCE

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_client.h"
#include "sctp_server.h"
#include "security.h"
#include "sync.h"
#include "telecom.h"
#include "udp_client.h"
#include "utils.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include "mtcp_api.h"
#include "mtcp_epoll.h"
#include "dpdk_api.h"
#include "cpu.h"
#include "debug.h"
#include <time.h>
/*----------------

//temp, mtcp-code. specific
/*#define PORTB 6000
#define CPORTBEG 7000
#define CPORTMAX 7000
#define IPADDRB "169.254.8.254"
#define IPADDRC "192.168.122.170"
#define MAXCON 10000
#define MAXEVENTS 10000
#define THC 10000
#define BPORTBEG 12000
#define BPORTEND 30000
*/
//

//#include "defport.h"


#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <time.h>
#include <unordered_map>

#define clientport 5000
#define MAX_EVENTS 2048
#define no_of_connections 1

using namespace std;


//variable declaration

//end of variable declaration

class RanContext {
public:
	/* EMM state 
	 * 0 - Deregistered
	 * 1 - Registered 
	 */	
	int emm_state; /* EPS Mobililty Management state */

	/* ECM state 
	 * 0 - Disconnected
	 * 1 - Connected 
	 * 2 - Idle 
	 */
	int ecm_state; /* EPS Connection Management state */

	/* UE id */
	uint64_t imsi; /* International Mobile Subscriber Identity */
	uint64_t guti; /* Globally Unique Temporary Identifier */
	string ip_addr;
	uint32_t enodeb_s1ap_ue_id; /* eNodeB S1AP UE ID */
	uint32_t mme_s1ap_ue_id; /* MME S1AP UE ID */

	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */
	vector<uint64_t> tai_list; /* Tracking Area Identifier list */
	uint64_t tau_timer;

	/* UE security context */
	uint64_t key; /* Primary key used in generating secondary keys */
	uint64_t k_asme; /* Key for Access Security Management Entity */
	uint64_t ksi_asme; /* Key Selection Identifier for Access Security Management Entity */
	uint64_t k_nas_enc; /* Key for NAS Encryption / Decryption */
	uint64_t k_nas_int; /* Key for NAS Integrity check */
	uint64_t nas_enc_algo; /* Idenitifier of NAS Encryption / Decryption */
	uint64_t nas_int_algo; /* Idenitifier of NAS Integrity check */
	uint64_t count;
	uint64_t bearer;
	uint64_t dir;

	/* EPS info, EPS bearer info */
	uint64_t apn_in_use; /* Access Point Name in Use */
	uint8_t eps_bearer_id; /* Evolved Packet System Bearer ID */
	uint8_t e_rab_id; /* Evolved Radio Access Bearer ID */
	uint32_t s1_uteid_ul; /* S1 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s1_uteid_dl; /* S1 Userplane Tunnel Endpoint Identifier - Downlink */

	/* Network Operator info */
	uint16_t mcc; /* Mobile Country Code */
	uint16_t mnc; /* Mobile Network Code */
	uint16_t plmn_id; /* Public Land Mobile Network ID */	
	uint64_t msisdn; /* Mobile Station International Subscriber Directory Number - Mobile number */
	uint16_t nw_capability;

	/* Parameters added for hand-over  */
	uint16_t eNodeB_id; /* EnodeB identifier */ // we could send this at all msg
	uint16_t handover_target_eNodeB_id; /* EnodeB identifier */

	bool inHandover; /* handover phase*/

	//when inHandover = true use the below uplink
	uint32_t indirect_s1_uteid_ul;
	// handover changes end/

	RanContext();
	void init(uint32_t);
	~RanContext();	
};

class EpcAddrs {
public:
	int mme_port;
	int sgw_s1_port;
	string mme_ip_addr;
	string sgw_s1_ip_addr;

	EpcAddrs();
	~EpcAddrs();
};

class Ran {
private:
	//EpcAddrs epc_addrs;
	//SctpClient mme_client;
	
	
public:
	/* Parameters added for hand-over  */

	bool inHandover; /* handover phase*/
 	//when inHandover = true use the below uplink
	uint32_t indirect_s1_uteid_ul;
	// handover changes end/
 
	/* Parameters added for hand-over  */
	uint16_t eNodeB_id; /* EnodeB identifier */
	uint16_t handover_target_eNodeB_id; /* EnodeB identifier */
	int handover_state;
	/* HO state
		 * 0 - Not in handover
		 * 1 - Handover initiated:for Source RAN
		 * 2 - Handover requested:for Target RAN
		 * 3 - Handover procedure at target done
		 * 4 - Handover at source done
		 */

	//ho methods
	/*void initiate_handover();
	void handle_handover(Packet pkt);
	void indirect_tunnel_complete(Packet pkt);
	void complete_handover();
	void request_tear_down(Packet pkt);*/
	//
 
	RanContext ran_ctx;
	Packet pkt;
	void init(int);
	//void set_crypt_context();
	//void set_integrity_context();
	//void store_packet(Packet & packet);
	//void showpack();
	//void conn_mme();
	//void initial_attach(mctx_t mctx, int);
	//bool authenticate(mctx_t mctx, int);
	//bool set_security(mctx_t mctx, int);
	//bool set_eps_session(TrafficMonitor&);
	//void transfer_data(int);
	//bool detach();	
};




int read_stream(mctx_t mctx, int conn_fd, uint8_t *buf, int len);

int write_stream(mctx_t mctx, int conn_fd, uint8_t *buf, int len);