#include "ic.hpp"
#include "vp_addr.hpp"

using namespace std;
using namespace tlm;
using namespace sc_core;
using namespace sc_dt;

interconnect::interconnect(sc_module_name name):
	sc_module(name),
	soc_cpu("ic_soc_cpu"),
	isoc_bram("ic_isoc_bram"),
	isoc_ip("ic_isoc_ip"),
	isoc_dma("ic_isoc_dma"),
	soc_dma("ic_soc_dma"),
	isoc_ram("ic_isoc_ram")
{
	soc_cpu.register_b_transport(this, &interconnect::b_transport_cpu);
	soc_dma.register_b_transport(this, &interconnect::b_transport_dma);
}

void interconnect::b_transport_dma(pl_t& pl, sc_core::sc_time& offset)
{
	offset += sc_time(10, SC_NS);
	isoc_ram->b_transport(pl, offset);
}

//Interpretacija tlm komunikacije u zavisnosti od adrese 
void interconnect::b_transport_cpu(pl_t& pl, sc_core::sc_time& offset)
{
	uint64 addr = pl.get_address();
	uint64 taddr;
	
	offset += sc_time(2, SC_NS);
	
	//ukoliko su globalne adrese iz opsega koji pokriva BRAM modul
	if(addr >= VP_ADDR_BRAM_HEAD_AND_TAIL && addr < VP_ADDR_BRAM_H)
	{
		//sve lokalne BRAM adrese se mogu predstaviti pomocu maksimalno 16 bita
		taddr = addr & 0x0000FFFF;
		pl.set_address(taddr);
		isoc_bram->b_transport(pl, offset);
	}
	//ukoliko su globalne adrese iz opsega koji pokriva IP modul
	else if(addr == VP_ADDR_IP_CFG_REG)
	{
		//sve lokalne IP adrese se mogu predstaviti pomocu 1 bita
		taddr = addr & 0x00000001;
		pl.set_address(taddr);
		isoc_ip->b_transport(pl, offset);
	}
	//ukoliko su globalne adrese iz opsega koji pokriva DMA modul
	else if(addr >= VP_ADDR_DMA && addr <= VP_ADDR_DMA_S2MM_DMACR)
	{
		taddr = addr & 0x000000FF;
		pl.set_address(taddr);
		isoc_dma->b_transport(pl, offset);
	}
	//ukoliko su globalne adrese iz opsega koji pokriva RAM modul
	else if(addr >= VP_ADDR_RAM && addr < VP_ADDR_RAM_H)
	{
		taddr = addr;
		pl.set_address(taddr);
		isoc_ram->b_transport(pl, offset);		
	}
	//u suprotnom
	else
	{
		SC_REPORT_ERROR("INTERCONNECT", "Invalid address!");
		pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
		return;
	}
	pl.set_address(addr);
}
