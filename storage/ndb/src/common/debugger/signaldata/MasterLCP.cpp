/*
   Copyright (C) 2003-2006 MySQL AB
    All rights reserved. Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/


#include <signaldata/PrimaryLCP.hpp>
#include <RefConvert.hpp>

static
void
print(char *buf, size_t buf_len, PrimaryLCPConf::State s){
  switch(s){
  case PrimaryLCPConf::LCP_STATUS_IDLE:
    BaseString::snprintf(buf, buf_len, "LCP_STATUS_IDLE");
    break;
  case PrimaryLCPConf::LCP_STATUS_ACTIVE:
    BaseString::snprintf(buf, buf_len, "LCP_STATUS_ACTIVE");
    break;
  case PrimaryLCPConf::LCP_TAB_COMPLETED:
    BaseString::snprintf(buf, buf_len, "LCP_TAB_COMPLETED");
    break;
  case PrimaryLCPConf::LCP_TAB_SAVED:
    BaseString::snprintf(buf, buf_len, "LCP_TAB_SAVED");
    break;
  }
}

NdbOut &
operator<<(NdbOut& out, const PrimaryLCPConf::State& s){
  static char buf[255];
  print(buf, sizeof(buf), s);
  out << buf;
  return out;
}

bool 
printPRIMARY_LCP_CONF(FILE * output, 
		     const Uint32 * theData, 
		     Uint32 len, 
		     Uint16 recBlockNo){
  
  PrimaryLCPConf * sig = (PrimaryLCPConf *)&theData[0];
  
  static char buf[255];
  print(buf, sizeof(buf), (PrimaryLCPConf::State)sig->lcpState);
  fprintf(output, " senderNode=%d failedNode=%d SenderState=%s\n",
	  sig->senderNodeId, sig->failedNodeId, buf);
  return true;
}

bool 
printPRIMARY_LCP_REQ(FILE * output, 
		    const Uint32 * theData, 
		    Uint32 len, 
		    Uint16 recBlockNo){
  
  PrimaryLCPReq * sig = (PrimaryLCPReq *)&theData[0];
  
  fprintf(output, " primaryRef=(node=%d, block=%d), failedNode=%d\n",
	  refToNode(sig->primaryRef), refToBlock(sig->primaryRef),
	  sig->failedNodeId);
  return true;
}

bool 
printPRIMARY_LCP_REF(FILE * output, 
		    const Uint32 * theData, 
		    Uint32 len, 
		    Uint16 recBlockNo){
  
  PrimaryLCPRef * sig = (PrimaryLCPRef *)&theData[0];  
  fprintf(output, " senderNode=%d failedNode=%d\n",
	  sig->senderNodeId, sig->failedNodeId);
  return true;
}
