#include "vp_addr.hpp"
#include "geo_dist.h"
#include "tb_vp.hpp"
#include "bram_mem.hpp"

using namespace sc_core;
using namespace sc_dt;
using namespace std;
using namespace tlm;

SC_HAS_PROCESS(tb_vp);

//Konstruktor klase
tb_vp::tb_vp(sc_module_name name):
	sc_module(name),
	isoc("tb_vp_isoc"),
	IRQ_F2P("IRQ_F2P")
{
	SC_THREAD(core0);
	sensitive << new_output_is_ready;
	
	SC_THREAD(core1);
	sensitive << new_locations_uploaded;
	
	SC_METHOD(gic);
	dont_initialize();
	sensitive << IRQ_F2P;
	
	SC_THREAD(IP_ISR);
	sensitive << ip_interrupt_event;
	
	SC_THREAD(DMA_ISR);
	sensitive << dma_interrupt_event;
	
	SC_METHOD(exit_simulation);
	dont_initialize();
	sensitive << end_of_simulation;
	
	index = 0;
}

void tb_vp::gic() 
{
	sc_uint<2> irq = IRQ_F2P.read();
	SC_REPORT_INFO("TB", "prekid registrovan!");
	
	cout << endl << irq[1] << irq[0] << endl;
	
	if (irq[1]) 
		dma_interrupt_event.notify(SC_ZERO_TIME);
	else if (irq[0]) 
		ip_interrupt_event.notify(SC_ZERO_TIME);
}

void tb_vp::IP_ISR()
{
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	
	while(1)
	{
		wait(ip_interrupt_event);
		
		pl_t pl;
		sc_time offset;
		
		unsigned char data_uint8[sizeof(uint8_t)]; 
		unsigned char data_uint32[sizeof(uint32_t)];
		
		pl.set_data_ptr(data_uint8);
		pl.set_address(VP_ADDR_IP_CFG_REG);
		pl.set_data_length(sizeof(uint8_t));
		pl.set_command(TLM_READ_COMMAND);
		pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		isoc->b_transport(pl, offset);
		assert(pl.get_response_status() == TLM_OK_RESPONSE);
		std::memcpy(&ip_register_content, data_uint8, sizeof(ip_register_content));
		   	
		qk.inc(sc_time(10, SC_NS));
		offset = qk.get_local_time();
		qk.set_and_sync(offset);
		
		// na osnovu stanja u kome se ip modul nalazi procesor obavlja razlicite akcije u prekidnoj rutini
		// ako je IP_END_BIT == 1 proces relaksacija je zavrsen
		SC_REPORT_INFO("TB", "IP prekid registrovan!");
		
		if(ip_register_content & 0x10)
		{
			ip_register_content &= 0xEF;
		
			pl.set_data_ptr(data_uint8);
			pl.set_address(VP_ADDR_IP_CFG_REG);
			pl.set_data_length(sizeof(uint8_t));
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint8, &ip_register_content, sizeof(ip_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			relaxations_are_done.notify();
		}
		// ako je IP_DMA_REQ_BIT == 1 i IP_DMA_STREAM_BIT je 0, 
		//	1. resetuj i pokreni dma modul
		//	2. upisi 1 na IP_DMA_STREAM_BIT i pocni sa upisom u dma modul
		else if(ip_register_content & 0x04 && !(ip_register_content & 0x08))
		{
			//procitaj stanje S2MM_DMACR registra
			pl.set_data_ptr(data_uint32);
			pl.set_address(VP_ADDR_DMA_S2MM_DMACR);
			pl.set_data_length(sizeof(uint32_t));
			pl.set_command(TLM_READ_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&dma_register_content, data_uint32, sizeof(dma_register_content));
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//1. resetovanje DMA modula
			dma_register_content |= 0x00000004;
			pl.set_data_ptr(data_uint32);
			pl.set_address(VP_ADDR_DMA_S2MM_DMACR);
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset); 
			
			//2. omogucavanje prekida
			dma_register_content &= 0xFFFFFFFB; //vracamo reset bit na 0 u pomocnoj promenjljivoj jer je reset izvrsen
			dma_register_content |= 0x00005000;
			pl.set_data_ptr(data_uint32);
			pl.set_address(VP_ADDR_DMA_S2MM_DMACR);
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//3. postavljanje adrese upisa u ram-u
			dma_register_content = VP_ADDR_LAST_GRAPH_LONGITUDE_ADDR + (index * 50) + 1;
			index++;
			pl.set_data_ptr(data_uint32);
			pl.set_address(VP_ADDR_DMA_S2MM_DA);
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//4. postavljanje broja podataka koji se upisuju
			dma_register_content = 50;
			pl.set_data_ptr(data_uint32);
			pl.set_address(VP_ADDR_DMA_S2MM_LENGTH);
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			// pokretanje S2MM kanala
			dma_register_content |= 0x00000002;
			pl.set_data_ptr(data_uint32);
			pl.set_data_length(sizeof(uint32_t));
			pl.set_address(VP_ADDR_DMA_S2MM_DMACR);
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset); 
			
			// upisi 1 na IP_DMA_STREAM_BIT i pocni sa upisom u dma modul
			ip_register_content &= 0xFB;
			ip_register_content |= 0x08; 
			
			pl.set_data_ptr(data_uint8);
			pl.set_address(VP_ADDR_IP_CFG_REG);
			pl.set_data_length(sizeof(uint8_t));
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint8, &ip_register_content, sizeof(ip_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
		}
	}
}

void tb_vp::DMA_ISR()
{
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	
	while(1)
	{
		wait(dma_interrupt_event);
		SC_REPORT_INFO("TB", "DMA prekid registrovan!");
		pl_t pl;
		sc_time offset;
		uint8_t ip_register_content;
		uint32_t dma_register_content;
		unsigned char data_uint8[sizeof(uint8_t)]; 
		unsigned char data_uint32[sizeof(uint32_t)];
		
		// gasenje S2MM kanala
		dma_register_content &= 0xFFFFFFFD;
		pl.set_data_ptr(data_uint32);
		pl.set_data_length(sizeof(uint32_t));
		pl.set_address(VP_ADDR_DMA_S2MM_DMACR);
		pl.set_command(TLM_WRITE_COMMAND);
		pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		std::memcpy(data_uint32, &dma_register_content, sizeof(dma_register_content));
		isoc->b_transport(pl, offset);
		assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
		qk.inc(sc_time(10, SC_NS));
		offset = qk.get_local_time();
		qk.set_and_sync(offset);   
		
		// citanje stanja IP registra u slucaju da je proces relaksacija gotov
		pl.set_data_ptr(data_uint8);
		pl.set_address(VP_ADDR_IP_CFG_REG);
		pl.set_data_length(sizeof(uint8_t));
		pl.set_command(TLM_READ_COMMAND);
		pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		isoc->b_transport(pl, offset);
		assert(pl.get_response_status() == TLM_OK_RESPONSE);
		std::memcpy(&ip_register_content, data_uint8, sizeof(ip_register_content));
		   	
		qk.inc(sc_time(10, SC_NS));
		offset = qk.get_local_time();
		qk.set_and_sync(offset);
		
		// ako je IP_END_BIT == 1 proces relaksacija je zavrsen
		if(ip_register_content & 0x10)
		{
			ip_register_content &= 0xEF;
		
			pl.set_data_ptr(data_uint8);
			pl.set_address(VP_ADDR_IP_CFG_REG);
			pl.set_data_length(sizeof(uint8_t));
			pl.set_command(TLM_WRITE_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint8, &ip_register_content, sizeof(ip_register_content));
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);

			relaxations_are_done.notify();
		}
	}
}

void tb_vp::exit_simulation()
{
	sc_stop();
}

void tb_vp::core0()
{
	std::ofstream ofs1("input_file.txt", std::ofstream::trunc);
	std::ofstream ofs2("output_file.txt", std::ofstream::trunc);	
	
	svr.Get("/", [this](const httplib::Request& req, httplib::Response& res) 
	{
        	std::ifstream file("../web/user_interface.html");
            	if(file) 
            	{
                	std::stringstream buffer;
                	buffer << file.rdbuf();
                	res.set_content(buffer.str(), "text/html");
            	} 
            	else 
            	{
                	res.status = 404;
                	res.set_content("user_interface.html nije pronađen", "text/plain");
            	}
        });
        
	svr.Get("/reset", [this](const httplib::Request& req, httplib::Response& res)
	{
		SC_REPORT_INFO("core0", "Reset zahtevan.");
		index = 0;
		
		// Pokušaj da obrišeš fajlove
		bool input_deleted  = (std::remove("input_file.txt")  == 0);
		bool output_deleted = (std::remove("output_file.txt") == 0);

		std::ostringstream response;
		response << "Reset OK. ";

		if (input_deleted)
			response << "input_file.txt obrisan. ";
		else
			response << "input_file.txt nije postojao ili nije obrisan. ";

		if (output_deleted)
			response << "output_file.txt obrisan.";
		else
			response << "output_file.txt nije postojao ili nije obrisan.";

		res.set_content(response.str(), "text/plain");
	});


        svr.Post("/upload", [this](const httplib::Request& req, httplib::Response& res)
        {
            	auto file = req.get_file_value("inputFile");

            	// Upisivanje u input_file.txt
            	std::ofstream out("input_file.txt");
            	if(!out)
            	{
           		res.status = 500;
                	res.set_content("Greska pri otvaranju input_file.txt", "text/plain");
                	return;
            	}
            	out << file.content;
            	out.close();
		
		new_locations_uploaded.notify(SC_ZERO_TIME);

            	wait(new_output_is_ready);

            	// Čitanje iz output_file.txt i formiranje JSON odgovora
            	std::ifstream in("output_file.txt");
            	if(!in) 
            	{
                	res.status = 500;
                	res.set_content("Greska pri otvaranju output_file.txt", "text/plain");
                	return;
            	}

            	std::string lat, lng;
            	std::ostringstream json;
            	json << "{ \"route\": [";

            	bool first = true;
            	while(in >> lat >> lng) 
            	{
                	if(!first)
                		json << ",";
                	json << "{ \"lat\": " << lat << ", \"lng\": " << lng << " }";
                	first = false;
            	}

            	json << "] }";
            	res.set_content(json.str(), "application/json");
        });
        
        svr.Get("/exit", [this](const httplib::Request& req, httplib::Response& res) {
    		SC_REPORT_INFO("core0", "Zahtev za izlaz primljen. Pozivam sc_stop().");
    		
    		res.set_content("Simulacija se gasi...", "text/plain");
		svr.stop();
		end_of_simulation.notify();
	});
	
	SC_REPORT_INFO("core0", "Server startovan na http://localhost:3000/");
        
        svr.listen("localhost", 3000);
}

void tb_vp::core1()
{
	/***************************************************************************************************************************************
		Istanciranje objekta klase tlm_utils::tlm_quantumkeeper; 	
	****************************************************************************************************************************************/
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();
	/****************************************************************************************************************************************/ 

	while(1)
	{
		wait(new_locations_uploaded);
	    	
		/****************************************************************************************************************************************
			Ucitavanje koordinata pocetne i krajnje lokacije iz fajla input_file.txt u promenljive start_lat, start_lon, end_lat, end_lon.
			Trazenje najblizeg temena pomocu naivnog pristupa.
			Zavrsetak simulacije u slucaju da fajl input_file.txt ne postoji ili ukoliko teme nije pronadjeno. 
		****************************************************************************************************************************************/	 
		std::ifstream input_file("input_file.txt");
		if (!input_file.is_open())
		{
			SC_REPORT_ERROR("core1", "Nije moguće otvoriti input_file.txt");
			end_of_simulation.notify();
		}
		
		float start_lat, start_lon, end_lat, end_lon;
		input_file >> start_lat >> start_lon >> end_lat >> end_lon;

		input_file.close();

		SC_REPORT_INFO("core1", "Poznate su lokacije!...");
		
		//********************************************************naive approach*****************************************************************
		unsigned start_vertex = invalid_id; 
		unsigned end_vertex = invalid_id;
		float start_distance_to_pivot;
		float end_distance_to_pivot; 
		float start_min_distance = inf_weight;
		float end_min_distance = inf_weight;
		float temp_start_graph_longitude;
		float temp_start_graph_latitude;
		float temp_end_graph_latitude;
		float temp_end_graph_longitude;
		unsigned radius = 500;
		uint8_t ip_register_content;
		sc_time offset;
		
		pl_t pl;
		unsigned char data_float[sizeof(float)]; 
		pl.set_data_ptr(data_float);
		
		for(uint64 i = 0; i < VERTEX_NUM; i++)
		{
			//Ucitavanje geografske sirine za startnu lokaciju iz RAM-a
		   	pl.set_address(VP_ADDR_RAM + i);
		   	pl.set_data_length(sizeof(float));
		   	pl.set_command(TLM_READ_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		   	isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&temp_start_graph_latitude, data_float, sizeof(temp_start_graph_latitude));
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//Ucitavanje geografske duzine za startnu lokaciju iz RAM-a
		   	pl.set_address(i + (VP_ADDR_LAST_GRAPH_LATITUDE_ADDR + 1));
		   	pl.set_data_length(sizeof(float));
		   	pl.set_command(TLM_READ_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		   	isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&temp_start_graph_longitude, data_float, sizeof(temp_start_graph_longitude));
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//Ucitavanje geografske sirine za krajnju lokaciju iz RAM-a
		   	pl.set_address(VP_ADDR_RAM + i);
		   	pl.set_data_length(sizeof(float));
		   	pl.set_command(TLM_READ_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		   	isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&temp_end_graph_latitude, data_float, sizeof(temp_end_graph_latitude));
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//Ucitavanje geografske duzine za krajnju lokaciju iz RAM-a
		   	pl.set_address(i + (VP_ADDR_LAST_GRAPH_LATITUDE_ADDR + 1));
		   	pl.set_data_length(sizeof(float));
		   	pl.set_command(TLM_READ_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
		   	isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&temp_end_graph_longitude, data_float, sizeof(temp_end_graph_longitude));
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			start_distance_to_pivot = geo_dist(start_lat, start_lon, temp_start_graph_latitude, temp_start_graph_longitude);
			end_distance_to_pivot = geo_dist(end_lat, end_lon, temp_end_graph_latitude, temp_end_graph_longitude);
			if(start_distance_to_pivot < start_min_distance && start_distance_to_pivot <= radius)
			{
				start_min_distance = start_distance_to_pivot;
				start_vertex = i;
			}
			if(end_distance_to_pivot < end_min_distance && end_distance_to_pivot <= radius)
			{
				end_min_distance = end_distance_to_pivot;
				end_vertex = i;
			}      
		}	   
		SC_REPORT_INFO("core1", "GeoPosition to node računica izvršena!...");
		
		//***************************************************************************************************************************************
		if (start_vertex == invalid_id || end_vertex == invalid_id) {
			SC_REPORT_ERROR("core1", "GeoPosition to node računica je naišla na problem: start_vertex == invalid_id ili end_vertex == invalid_id!...");
			new_output_is_ready.notify();
		}
		else if(start_vertex == end_vertex)
		{
			SC_REPORT_INFO("core1", "-----------------------------------------");
			SC_REPORT_INFO("core1", "Pocetna i krajnja lokacija su ista tacka.");
			SC_REPORT_INFO("core1", "-----------------------------------------");
			new_output_is_ready.notify();
		}
		else
		{
			SC_REPORT_INFO("core1", "Inicijalizacija BRAM modula!...");
			/***************************************************************************************************************************************
				Inicijalizacija BRAM modula 
			****************************************************************************************************************************************/	
			unsigned char data_uint32[sizeof(uint32_t)]; 
			pl.set_data_ptr(data_uint32);	

		   	//Upis cost_from_start_vertex i vertex_is_visited podataka u BRAM
		   	uint32_t i = 0;
		   	uint32_t temp_cost, temp_visited;
		   	uint32_t bram_cell_32bit = 0;
		    	uint32_t address = VP_ADDR_BRAM_COST_AND_VISITED_VERTEX;
		   	while(address >= VP_ADDR_BRAM_COST_AND_VISITED_VERTEX && address < VP_ADDR_BRAM_H)
		   	{
		   		temp_visited = false;
		   		if(i == start_vertex)
					temp_cost = 0;
				else
					temp_cost = inf_weight;
		   		
		   		bram_cell_32bit = (temp_visited << 31) | (temp_cost);
		   	   	pl.set_address(address);
		   	   	pl.set_data_length(sizeof(bram_cell_32bit));
		  		pl.set_command(TLM_WRITE_COMMAND);
		   	   	std::memcpy(data_uint32, &bram_cell_32bit, sizeof(bram_cell_32bit));
		   	   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			   	isoc->b_transport(pl, offset);
		   	   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   		qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
		   		i++;
		   		address++;
		   	}   
		   	SC_REPORT_INFO("core1", "Kraj inicijalizacije BRAMA!...");
		   	/**************************************************Kraj inicijalizacije BRAMA***********************************************************/	
			
			/***************************************************************************************************************************************
				U ovom trenutku su poznati svi potrebni podaci za pocetak pretrage optimalne putanje. 
				U ovom delu se IP bloku signalizira da moze da zapocne sa relaksacijama. Pocetno teme je poznato jer jedino ima udaljenost
				od pocetnog temena jednako nuli(cost_from_start_vertex). 
			****************************************************************************************************************************************/
			SC_REPORT_INFO("core1", "Upis u Ip konfiguracioni registar IP_CFG_REG!...");
			unsigned char data_uint8[sizeof(uint8_t)]; 
			pl.set_data_ptr(data_uint8);
			
			//resetuj ip modul za sledeci upit
			ip_register_content = 0x01;
			
			address = VP_ADDR_IP_CFG_REG;
		   	pl.set_address(address);
		   	pl.set_data_length(sizeof(uint8_t));
		   	pl.set_command(TLM_WRITE_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	std::memcpy(&ip_register_content, data_uint8, sizeof(ip_register_content));
		   	
		   	qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			
			//pocni sa procesom relaksacija
			ip_register_content = 0x02;
			
			pl.set_command(TLM_WRITE_COMMAND);
		   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			std::memcpy(data_uint8, &ip_register_content, sizeof(ip_register_content));
			isoc->b_transport(pl, offset);
		   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
		   	
		   	qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
		   	SC_REPORT_INFO("core1", "Upis u Ip konfiguracioni registar IP_CFG zavrsen!... Pocinje se sa procesom relaksacija!...");
		   	/***************************************************************************************************************************************/
		   	
		   	/***************************************************************************************************************************************
		   		Implementacija procesa relaksacije je u IP modulu (relaxationProcess iz izvrsne specifikacije)
		   	****************************************************************************************************************************************/
		    	wait(relaxations_are_done);
			/********************************************************DIJKSTRA'S_OUTPUT**************************************************************/

			SC_REPORT_INFO("core1", "Proces relaksacija je gotov!...");
			
			node_id_t temp_relaxed, temp_relaxing;
			uint32_t temp_relaxation;
			address = VP_ADDR_LAST_GRAPH_LONGITUDE_ADDR + 1;
			pl.set_data_ptr(data_uint32);
			pl.set_address(address);
			pl.set_data_length(sizeof(uint32_t));
			pl.set_command(TLM_READ_COMMAND);
			pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
			isoc->b_transport(pl, offset);
			assert(pl.get_response_status() == TLM_OK_RESPONSE);
			std::memcpy(&temp_relaxation, data_uint32, sizeof(uint32_t));
			qk.inc(sc_time(10, SC_NS));
			offset = qk.get_local_time();
			qk.set_and_sync(offset);
			temp_relaxed = temp_relaxation & 0x00003FFF;
			temp_relaxing = (temp_relaxation >> 18) & 0x00003FFF;
			while(temp_relaxing != start_vertex && temp_relaxed != end_vertex)
			{
				address++;
				pl.set_address(address);
				pl.set_data_length(sizeof(uint32_t));
				pl.set_command(TLM_READ_COMMAND);
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
				isoc->b_transport(pl, offset);
				assert(pl.get_response_status() == TLM_OK_RESPONSE);
				std::memcpy(&temp_relaxation, data_uint32, sizeof(uint32_t));
				qk.inc(sc_time(10, SC_NS));
				offset = qk.get_local_time();
				qk.set_and_sync(offset);
				temp_relaxed = temp_relaxation & 0x00003FFF;
				temp_relaxing = (temp_relaxation >> 18) & 0x00003FFF;
			}
			
			if(temp_relaxing == start_vertex)
			{
				SC_REPORT_INFO("core1", "-------------------------------------------------");
				SC_REPORT_INFO("core1", "Saobracajnica izmedju unetih lokacija ne postoji.");
				SC_REPORT_INFO("core1", "-------------------------------------------------");
				new_output_is_ready.notify();
			}
			else
			{
				node_id_t this_vertex = temp_relaxed;
				node_id_t got_relaxed_by = temp_relaxing;

				std::vector<node_id_t> path;
				path.insert(path.begin(), this_vertex);
				path.insert(path.begin(), got_relaxed_by);
				while(got_relaxed_by != start_vertex)
				{
					while(got_relaxed_by != temp_relaxed)
					{
						address++;
						pl.set_address(address);
						pl.set_data_length(sizeof(uint32_t));
						pl.set_command(TLM_READ_COMMAND);
						pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
						isoc->b_transport(pl, offset);
						assert(pl.get_response_status() == TLM_OK_RESPONSE);
						std::memcpy(&temp_relaxation, data_uint32, sizeof(uint32_t));
						qk.inc(sc_time(10, SC_NS));
						offset = qk.get_local_time();
						qk.set_and_sync(offset);
						temp_relaxed = temp_relaxation & 0x00003FFF;
						temp_relaxing = (temp_relaxation >> 18) & 0x00003FFF;
					}
					
					this_vertex = temp_relaxed;
					got_relaxed_by = temp_relaxing;
					path.insert(path.begin(), got_relaxed_by);
				}
				
				SC_REPORT_INFO("core1", "-------------");
				SC_REPORT_INFO("core1", "Srecan put :)");
				SC_REPORT_INFO("core1", "-------------");
				
				std::ofstream output_file("output_file.txt");
				if (!output_file.is_open())
				{
					SC_REPORT_ERROR("core1", "Nije moguće otvoriti output_file.txt");
					end_of_simulation.notify();
				}
				
				/*************************************************************************************************************************************/
				pl.set_data_ptr(data_float);
				float current_lat, current_lon;
				unsigned path_iter = 0;
				while(path_iter != path.size()) 
				{
					//Ucitavanje geografske sirine iz RAM-a
				   	pl.set_address(VP_ADDR_RAM + path[path_iter]);
				   	pl.set_data_length(sizeof(float));
				   	pl.set_command(TLM_READ_COMMAND);
				   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
				   	isoc->b_transport(pl, offset);
				   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
				   	std::memcpy(&current_lat, data_float, sizeof(float));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					//Ucitavanje geografske duzine iz RAM-a	
				   	pl.set_address(path[path_iter] + (VP_ADDR_LAST_GRAPH_LATITUDE_ADDR + 1));
				   	pl.set_data_length(sizeof(float));
				   	pl.set_command(TLM_READ_COMMAND);
				   	pl.set_response_status(TLM_INCOMPLETE_RESPONSE);
				   	isoc->b_transport(pl, offset);
				   	assert(pl.get_response_status() == TLM_OK_RESPONSE);
				   	std::memcpy(&current_lon, data_float, sizeof(float));
					qk.inc(sc_time(10, SC_NS));
					offset = qk.get_local_time();
					qk.set_and_sync(offset);
					
					output_file << current_lat << " " << current_lon << endl;
					
					path_iter++;
				}
				
				new_output_is_ready.notify();
			}
		}
	}	
}
