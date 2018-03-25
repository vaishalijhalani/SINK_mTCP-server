#include "extra.h"

//Rancontext functions

RanContext::RanContext() {
	emm_state = 0; 
	ecm_state = 0; 
	imsi = 0; 
	guti = 0; 
	ip_addr = "";
	enodeb_s1ap_ue_id = 0; 
	mme_s1ap_ue_id = 0; 
	tai = 1; 
	tau_timer = 0;
	key = 0; 
	k_asme = 0; 
	ksi_asme = 7; 
	k_nas_enc = 0; 
	k_nas_int = 0; 
	nas_enc_algo = 0; 
	nas_int_algo = 0; 
	count = 1;
	bearer = 0;
	dir = 0;
	apn_in_use = 0; 
	eps_bearer_id = 0; 
	e_rab_id = 0; 
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	mcc = 1; 
	mnc = 1; 
	plmn_id = g_telecom.get_plmn_id(mcc, mnc);
	msisdn = 0; 
	nw_capability = 1;
}

void RanContext::init(uint32_t arg) {
	enodeb_s1ap_ue_id = arg;
	key = arg;
	msisdn = 9000000000 + arg;
	imsi = g_telecom.get_imsi(plmn_id, msisdn);
}

EpcAddrs::EpcAddrs() {
	mme_port = 5000;
	sgw_s1_port = 0;
	mme_ip_addr = "169.254.9.8";	
	sgw_s1_ip_addr = "";
}

EpcAddrs::~EpcAddrs() {

}

RanContext::~RanContext() {

}

//Ran class function

void Ran::init(int arg) {
	ran_ctx.init(arg);
}

/*void Ran::store_packet(Packet & packet)
{
	pkt = packet;
	
}

void Ran::showpack()
{
	pkt.extract_s1ap_hdr();
	cout << "packetid " <<  pkt.s1ap_hdr.mme_s1ap_ue_id << endl;
}

void Ran::initial_attach(mctx_t mctx, int ran_fd) {
	int returnval;
	pkt.clear_pkt();
	pkt.append_item(ran_ctx.imsi);
	pkt.append_item(ran_ctx.tai);
	pkt.append_item(ran_ctx.ksi_asme);
	pkt.append_item(ran_ctx.nw_capability);
	pkt.prepend_s1ap_hdr(1, pkt.len, ran_ctx.enodeb_s1ap_ue_id, 0);
	pkt.prepend_len();
	returnval = write_stream(mctx, ran_fd, pkt.data, pkt.len);
	//cout << "return form write " << returnval;
	if(returnval < 0)
	{
		cout<<"Error: Cant send to RAN"<<endl;
		exit(-1);
	}
	TRACE(cout << "ran_initialattach:" << " request sent for ran: " << ran_ctx.imsi << endl;)
}



bool Ran::authenticate(mctx_t mctx, int ran_fd) 
{
	
	uint64_t autn_num;
	uint64_t xautn_num;
	uint64_t rand_num;
	uint64_t sqn;
	uint64_t res;
	uint64_t ck;
	uint64_t ik;
	int returnval;
	int pkt_len;
	/*Packet pkt;
	memcpy(pkt.data, buf, pkt_len);
	pkt.len = pkt_len;
	//mme_client.rcv(pkt);
	if (pkt.len <= 0) {
		return false;
	}
	//TRACE(cout << "ran_authenticate: " << " received request for ran: " << ran_ctx.imsi << endl;)

	//pkt = packet;
	if (pkt.len <= 0) {
		return false;
	}
	pkt.extract_s1ap_hdr();
    cout << "packetid " <<  pkt.s1ap_hdr.mme_s1ap_ue_id << endl;

	ran_ctx.mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
	pkt.extract_item(xautn_num);
	pkt.extract_item(rand_num);
	pkt.extract_item(ran_ctx.ksi_asme);
	
	TRACE(cout << "ran_authenticate: " << " autn: " << xautn_num << " rand: " << rand_num  << " ksiasme: " << ran_ctx.ksi_asme << ": " << ran_ctx.imsi << endl;)
	sqn = rand_num + 1;
	res = ran_ctx.key + sqn + rand_num;
	autn_num = res + 1;
	cout << autn_num << xautn_num << endl;
	if (autn_num != xautn_num) {
		TRACE(cout << "ran_authenticate:" << " authentication of MME failure: " << ran_ctx.imsi << endl;)
		return false;
	}
	TRACE(cout << "ran_authenticate:" << " autn success: " << ran_ctx.imsi << endl;)
	ck = res + 2;
	ik = res + 3;
	ran_ctx.k_asme = ck + ik + sqn + ran_ctx.plmn_id;
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_s1ap_hdr(2, pkt.len, ran_ctx.enodeb_s1ap_ue_id, ran_ctx.mme_s1ap_ue_id);
	pkt.prepend_len();
	returnval = write_stream(mctx, ran_fd, pkt.data, pkt.len);
	
	if(returnval < 0)
	{
		cout<<"Error: Cant send to RAN"<<endl;
		exit(-1);
	}
	TRACE(cout << "ran_authenticate:" << " autn response sent to mme: " << ran_ctx.imsi << endl;)
	return true;
}
bool Ran::set_security(mctx_t mctx, int ran_fd) {
	uint8_t *hmac_res;
	uint8_t *hmac_xres;
	bool res;
	int returnval;

	//mme_client.rcv(pkt);
	if (pkt.len <= 0) {
		return false;
	}	


	hmac_res = g_utils.allocate_uint8_mem(HMAC_LEN);
	hmac_xres = g_utils.allocate_uint8_mem(HMAC_LEN);
	TRACE(cout << "ran_setsecurity: " << " received request for ran: " << pkt.len << ": " << ran_ctx.imsi << endl;)
	pkt.extract_s1ap_hdr();
	if (HMAC_ON) {
		g_integrity.rem_hmac(pkt, hmac_xres);
	}

	ran_ctx.mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
	pkt.extract_item(ran_ctx.ksi_asme);
	pkt.extract_item(ran_ctx.nw_capability);
	pkt.extract_item(ran_ctx.nas_enc_algo);
	pkt.extract_item(ran_ctx.nas_int_algo);
	set_crypt_context();
	set_integrity_context();
	if (HMAC_ON) {
		g_integrity.get_hmac(pkt.data, pkt.len, hmac_res, ran_ctx.k_nas_int);
		res = g_integrity.cmp_hmacs(hmac_res, hmac_xres);
		if (res == false) {
			TRACE(cout << "ran_setsecurity:" << " hmac security mode command failure: " << ran_ctx.imsi << endl;)
			g_utils.handle_type1_error(-1, "hmac error: ran_setsecurity");
		}
	}
	TRACE(cout << "ran_setsecurity:" << " security mode command success: " << ran_ctx.imsi << endl;)
	res = true;
	pkt.clear_pkt();
	pkt.append_item(res);
	if (ENC_ON) {
		g_crypt.enc(pkt, ran_ctx.k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, ran_ctx.k_nas_int);
	}
	pkt.prepend_s1ap_hdr(3, pkt.len, ran_ctx.enodeb_s1ap_ue_id, ran_ctx.mme_s1ap_ue_id);
	//mme_client.snd(pkt);
	pkt.prepend_len();
	returnval = write_stream(mctx, ran_fd, pkt.data, pkt.len);
	
	if(returnval < 0)
	{
		cout<<"Error: Cant send to RAN"<<endl;
		exit(-1);
	}
	TRACE(cout << "ran_setsecurity:" << " security mode complete sent to mme: " << pkt.len << ": " << ran_ctx.imsi << endl;)
	free(hmac_res);
	free(hmac_xres);
	return true;
}

void Ran::set_crypt_context() {
	ran_ctx.k_nas_enc = ran_ctx.k_asme + ran_ctx.nas_enc_algo + ran_ctx.count + ran_ctx.bearer + ran_ctx.dir + 1;
}

void Ran::set_integrity_context() {
	ran_ctx.k_nas_int = ran_ctx.k_asme + ran_ctx.nas_int_algo + ran_ctx.count + ran_ctx.bearer + ran_ctx.dir + 1;
}
/*
bool Ran::set_eps_session(TrafficMonitor &traf_mon) {
	bool res;
	uint64_t k_enodeb;
	int tai_list_size;

	mme_client.rcv(pkt);
	if (pkt.len <= 0) {
		return false;
	}
	TRACE(cout << "ran_setepssession:" << " attach accept received from mme: " << pkt.len << ": " << ran_ctx.imsi << endl;)
	pkt.extract_s1ap_hdr();
	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, ran_ctx.k_nas_int);
		if (res == false) {
			TRACE(cout << "ran_setepssession:" << " hmac attach accept failure: " << ran_ctx.imsi << endl;)
			g_utils.handle_type1_error(-1, "hmac error: ran_setepssession");
		}
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, ran_ctx.k_nas_enc);	
	}
	pkt.extract_item(ran_ctx.guti);
	pkt.extract_item(ran_ctx.eps_bearer_id);
	pkt.extract_item(ran_ctx.e_rab_id);
	pkt.extract_item(ran_ctx.s1_uteid_ul);
	pkt.extract_item(k_enodeb);
	pkt.extract_item(ran_ctx.nw_capability);
	pkt.extract_item(tai_list_size);
	pkt.extract_item(ran_ctx.tai_list, tai_list_size);
	pkt.extract_item(ran_ctx.tau_timer);
	pkt.extract_item(ran_ctx.ip_addr);
	pkt.extract_item(epc_addrs.sgw_s1_ip_addr);
	pkt.extract_item(epc_addrs.sgw_s1_port);
	pkt.extract_item(res);	
	if (res == false) {
		TRACE(cout << "ran_setepssession:" << " attach request failure: " << ran_ctx.imsi << endl;)
		return false;
	}	
	traf_mon.update_uplink_info(ran_ctx.ip_addr, ran_ctx.s1_uteid_ul, epc_addrs.sgw_s1_ip_addr, epc_addrs.sgw_s1_port);
	ran_ctx.s1_uteid_dl = ran_ctx.s1_uteid_ul;
	pkt.clear_pkt();
	pkt.append_item(ran_ctx.eps_bearer_id);
	pkt.append_item(ran_ctx.s1_uteid_dl);
	if (ENC_ON) {
		g_crypt.enc(pkt, ran_ctx.k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, ran_ctx.k_nas_int);
	}
	pkt.prepend_s1ap_hdr(4, pkt.len, ran_ctx.enodeb_s1ap_ue_id, ran_ctx.mme_s1ap_ue_id);
	mme_client.snd(pkt);
	TRACE(cout << "ran_setepssession:" << " attach complete sent to mme: " << pkt.len << ": " << ran_ctx.imsi << endl;)
	ran_ctx.emm_state = 1;
	ran_ctx.ecm_state = 1;
	return true;
}*/

int read_stream(mctx_t mctx, int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int read_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}
	while (1) {
		read_bytes = mtcp_read(mctx, conn_fd, buf + ptr, remaining_bytes);
		if (read_bytes <= 0) {
			retval = read_bytes;
			break;
		}
		ptr += read_bytes;
		remaining_bytes -= read_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}

int write_stream(mctx_t mctx, int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int written_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}	
	while (1) {
		written_bytes = mtcp_write(mctx, conn_fd, buf + ptr, remaining_bytes);
		if (written_bytes <= 0) {
			retval = written_bytes;
			break;
		}
		ptr += written_bytes;
		remaining_bytes -= written_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}
