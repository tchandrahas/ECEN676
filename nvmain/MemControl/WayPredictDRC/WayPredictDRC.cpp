/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
*
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author list:
*   Matt Poremba    ( Email: mrp5060 at psu dot edu
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/
#include "WayPredictDRC.h"
using namespace NVM;

WayPredictDRC::WayPredictDRC( )
{
    //translator->GetTranslationMethod( )->SetOrder( 5, 1, 4, 3, 2, 6 );
    // initiate the member of the class
  
    main_memory = NULL;
    dram_cache = NULL;
    miss_count = 0;
    hit_count = 0;
    evict_count = 0;
    write_hit=0;
    read_hit = 0;
    fill_count=0;
    read_time = 1;
    write_time = 1;
    hit_rate = -1.0;
    //default set of dram cache
    cache_line_size = 64;
    cache_assoc = 1;	//default cache associate
 	  //default cache capacity is 128MB
    cache_capacity = NVM::Power( 2 ,27 );
    
    read_miss = 0 ;
    write_miss = 0;
    
    write_allocate=true;
    perfect_fill = true;
    drcQueueSize=32;
    InitQueues( 1 );
    drcQueue = &(transactionQueues[0]);
    
        //set dram cache for main memory
    SetDRAMCache(dynamic_cast<AbstractDRAMCache*>(this));
    std::cout << "Constructor for Way Predict DRAM Cache executed sucessfully" << std::endl;
}

WayPredictDRC::~WayPredictDRC( )
{
  // deallocate the memory given to structures based on what mode is executed

  if(main_memory)
	{
    delete main_memory;
    main_memory = NULL;
	}

  if(dram_cache)
	{
    delete dram_cache;
    dram_cache = NULL;
	}

	if(drcQueue)
	{
		delete drcQueue;
		drcQueue = NULL;
	}
}
/*
 * read config , assign value for some basic vars
 * related-configuration:
 * @CacheCapacity: size of dram cache( unit can be B,KB,MB,GB,MB)
 * @CacheAssoc: associate of dram cache
 * @CacheLineSize: size of cache line
 * @ReadTime: dram read time(unit is cycle)
 * @WriteTime: dram Write time( unit is cycle)
 * @DRCQueueSize: dram cache queue to store commands
 * @PerfectFills: whether issue request firstly ? false: place commands in the queue, issue command according to priority
 * @WriteAllocate: write allocate(when write miss , fetch memory block to dram cache from main memory)?
 * 				   no-write-allocate ?( when write miss , write through main memory directly)
 *
 */
void WayPredictDRC::SetConfig( Config *conf, bool CreateChildren )
{
  // Scan all the way associative cache related parameters
  std::string tmp_str;
	if( conf->KeyExists("CacheCapacity"))
	{
		tmp_str = conf->GetString("CacheCapacity");
		this->cache_capacity = NVM::TransToBytes( tmp_str );
	}

	if( conf->KeyExists("CacheAssoc"))
	{
		this->cache_assoc = conf->GetValue("CacheAssoc");
	}

	if( conf->KeyExists("CacheLineSize"))
	{
		tmp_str = conf->GetString("CacheLineSize");
		this->cache_line_size = NVM::TransToBytes(tmp_str);
	}

	if( conf->KeyExists("ReadTime"))
	{
		this->read_time = conf->GetValue("ReadTime");
	}
	if( conf->KeyExists("WriteTime"))
	{
		this->write_time = conf->GetValue("WriteTime");
	}
	if( conf->KeyExists( "PerfectFills" ) && conf->GetString( "PerfectFills" ) == "true" )
		perfect_fill = true;
	if(conf->KeyExists("DRCQueueSize"))
			drcQueueSize = conf->GetValue("DRCQueueSize");
	if(conf->KeyExists( "WriteAllocate")&& conf->GetString("WriteAllocate")=="true")
	{
		write_allocate = true;
	}
	//calculate number of cache sets
	this->cache_sets = this->cache_capacity/(cache_line_size*cache_assoc);
	//create cache
	dram_cache = new CacheImpl(cache_assoc , cache_sets, cache_line_size ,read_time ,write_time );
  // Set the memory controller related parameters
  MemoryController::SetConfig( conf, CreateChildren );
}
uint64_t WayPredictDRC::GetDRAMHitTime()
{
	return dram_hit_time;
}

uint64_t WayPredictDRC::GetDRAMMissTime()
{
	return dram_miss_time;
}

void WayPredictDRC::SetDRAMCache(AbstractDRAMCache* dram_cache)
{
}

// set the pointer to main memory
void WayPredictDRC::SetMainMemory( NVMain* memory)
{
	this->main_memory = memory;
}

bool WayPredictDRC::IssueFunctional(NVMainRequest* req)
{
	bool is_hit = false;
	CacheBlock* tmp_blk = dram_cache->FindBlock(&(req->address));
	if( tmp_blk )
		   is_hit = true;
	return is_hit;
}

bool WayPredictDRC::IssueAtomic( NVMainRequest *req )
{
  CacheBlock* tmp_blk;
  uint64_t set_id , way_id;
  tmp_blk = dram_cache->FindBlock(&(req->address),set_id , way_id);
  //hit
  if( tmp_blk )
  {
    hit_count++;
    if(req->type==WRITE || req->type==WRITE_PRECHARGE)
      write_hit++;
    if(req->type==READ || req->type==READ_PRECHARGE)
      read_hit++;
  }
  else
  {
    miss_count++;
    fill_count++;
    if( (tmp_blk = dram_cache->FindVictim(&(req->address),set_id , way_id)))
    {
      //set full,need evict
      if((way_id==cache_assoc-1)&&(tmp_blk->IsValid()))
      {
        dram_cache->Evict( tmp_blk);
        evict_count++;
      }
    }
    dram_cache->Install(&(req->address),tmp_blk , &(req->data));
  }
  return true;
}

/*
 * issue request with timing model
 */
bool WayPredictDRC::IssueCommand( NVMainRequest* req)
{
		if( (req->owner==this)&& (req->type == WRITE || req->type == WRITE_PRECHARGE))
		{
			//will call this->RequestComplete(req) by order
			GetEventQueue()->InsertEvent(EventResponse , this , req , GetEventQueue()->GetCurrentCycle());
		}
		else
		{
			Enqueue(0,req);
		}
	return true;
}

bool WayPredictDRC::RequestComplete( NVMainRequest *req )
{
  bool ret = false;

	if( req->type == REFRESH)
	{
			ProcessRefreshPulse(req);
	}
	else if( req->owner == this)
	{
		if(req->tag==DRC_FILL_CACHE)
		{
			FillCache(req);
		}
		if(req->tag == DRC_MEMFETCH)
		{
			MemFetch(req);
			assert(outstanding_memfetch.count( req )>0);
			NVMainRequest* original_req = outstanding_memfetch[req];
			CacheBlock* tmp_blk = dram_cache->FindBlock(&(original_req->address));
			if( tmp_blk )
			{
				if( write_allocate &&(original_req->type==WRITE || original_req->type==WRITE_PRECHARGE))
				{
					dram_cache->Write(&(original_req->address),tmp_blk , &(original_req->data));
				}
				else if(original_req->type==READ||original_req->type==READ_PRECHARGE)
				{

					dram_cache->Read(&(original_req->address),tmp_blk , &(original_req->data));
				}
			}

			outstanding_memfetch.erase( req);
			GetParent()->RequestComplete( original_req);
		}

		if(req->tag == DRC_MIGRATION_MEMFETCH)
		{
			MemFetch(req);
			assert(outstanding_memfetch.count( req )>0);
			NVMainRequest* original_req = outstanding_memfetch[req];
			outstanding_memfetch.erase( req);
			main_memory->RequestComplete( original_req);
		}
		ret = true;
		delete req;
	}
	else
	{
		if(req->type==WRITE|| req->type==WRITE_PRECHARGE||req->type==READ||req->type==READ_PRECHARGE || req->tag==MIGRATION)
		{
			IssueToOtherModule(req);
			ret = false;
		}
		else
		{
			NVM::Fatal("unknown request type (generally are READ/READ_PRECHARGE/WRITE/WRITE_PRECHARGE)");
		}
	}
	return ret;
}

/*
 * intercept request, set its owner to this, tag to 'DRC_FILL_CACHE', type to WRITE/WRITE_PRECHARGE,
 * then issue the intercepted request
 */
void WayPredictDRC::MemFetch(NVMainRequest* req)
{
	NVMainRequest* fetch_req = new NVMainRequest();
	*fetch_req = *req;
	fetch_req->owner = this;
	fetch_req->tag = DRC_FILL_CACHE;
	#ifdef CLOSED_BANK
	fetch_req->type = WRITE_PRECHARGE;
	#else
	fetch_req->type = WRITE;
	#endif
	fetch_req->arrivalCycle = GetEventQueue()->GetCurrentCycle();
	this->IssueCommand(fetch_req );	//DRC_FILL
}


/*
 * fill data taken by req to dram cache
 * 1. if victim cache block is dirty , write back victim block to main memory firstly, then install new block to dram cache
 * 2. if victim cache block is not dirty , install new block to dram cache directly
 */
void WayPredictDRC::FillCache(NVMainRequest* req)
{

	bool dirty;
	NVMainRequest* write_back_req;
	uint64_t set_id , way_id;

	CacheBlock* victim_blk = dram_cache->FindVictim( &req->address , set_id , way_id);
	if( victim_blk->IsValid() )
	{
		//evict block , install new block
		//evict dirty,need write back
		if( (dirty = dram_cache->Evict(victim_blk )) )
		{
			write_back_req = new NVMainRequest();
			*write_back_req = *req;
			write_back_req->tag = WRITE_BACK;
			write_back_req->owner = this;
			#ifdef CLOSED_BANK
			write_back_req->type = WRITE_PRECHARGE;
			#else
			write_back_req->type = WRITE;
			#endif
			write_back_req->address.SetPhysicalAddress( dram_cache->GetPA( victim_blk->tag , set_id)); //set write back address

			if( !(write_back_req->data.rawData) )
			{
				write_back_req->data.rawData = new uint8_t[victim_blk->data->GetSize()];
			}
			memcpy( write_back_req->data.rawData , victim_blk->data->rawData , victim_blk->data->GetSize() );
			write_back_req->arrivalCycle = GetEventQueue()->GetCurrentCycle();

			main_memory->IssueCommand( write_back_req);
			wb_count++;
		    evict_count++;
		}
	}
			victim_blk->Invalidate();
			fill_count++;
			dram_cache->Install( &(req->address) , victim_blk , &(req->data) );
}

/*
 * get read/write(read_precharge/write_precharge) request from other module;
 * when read/write hit , read/write directly
 * when read/write-allocate miss , read demanded block from main memory to dram cache , then read/write
 * when no-write-allocate miss, issue request to main memory directly
 */
void WayPredictDRC::IssueToOtherModule(NVMainRequest* req)
{
		CacheBlock* tmp_blk = dram_cache->FindBlock(&(req->address));
		if( tmp_blk )
		{
			if(req->type == WRITE || req->type == WRITE_PRECHARGE)
			{
				write_hit++;
				GetParent()->RequestComplete( req );
			}
			else if( (req->type==READ|| req->type==READ_PRECHARGE)&& req->tag !=MIGRATION)
			{
				read_hit++;
				GetParent()->RequestComplete( req );
			}
			else if( (req->type==READ || req->type==READ_PRECHARGE) && (req->tag==MIGRATION) )
			{
				main_memory->RequestComplete( req );

			}
			hit_count++;
		}
		//read/write miss,intercept request to memory read
		else
		{
		   if( write_allocate || req->type == READ || req->type == READ_PRECHARGE)
		   {
				NVMainRequest* mm_req = new NVMainRequest();
				*mm_req = *req;
				mm_req->arrivalCycle = GetEventQueue()->GetCurrentCycle();
				mm_req->owner = this;
				if( req->tag != MIGRATION )
					mm_req->tag = DRC_MEMFETCH;
				else
				{
					mm_req->tag = DRC_MIGRATION_MEMFETCH;
				}
				mm_req->type = READ;
				//because main memory will call dram cache's RequestComplete(GetParent()->RequestComplete) function , so there is no need to call FillCache
				main_memory->IssueCommand( mm_req );

				outstanding_memfetch.insert(std::pair<NVMainRequest* , NVMainRequest*>(mm_req , req ));
				if( req->type==READ||req->type==READ_PRECHARGE)
				{
					read_miss++;
				}
				else if(req->type==WRITE||req->type==WRITE_PRECHARGE)
				{
					write_miss++;
				}
		   }

		   else if( !write_allocate && (req->type==WRITE||req->type==WRITE_PRECHARGE))
		   {
			  main_memory->IssueCommand( req );
			  write_miss++;
		   }
			miss_count++;
		}
}

void WayPredictDRC::Cycle( ncycle_t steps )
{
  NVMainRequest* next_req;
  //starved request first
  if(!FindStarvedRequest(*drcQueue, &next_req ) )
  {
    //row buffer hit secondly
    if(!FindRowBufferHit(*drcQueue, &next_req))
    {
      //oldest ready request thirdly
      if(!FindOldestReadyRequest(*drcQueue, &next_req))
      {	//lastly, closed bank request
        if(!FindClosedBankRequest(*drcQueue,&next_req))
        {
          next_req=NULL;
        }
      }
    }
  }
//NVM::DebugOutput(next_req);
if(next_req)
{
  IssueMemoryCommands(next_req);
}
CycleCommandQueues();
MemoryController::Cycle(steps);
}
//stats related
/*
 *
 */
void WayPredictDRC::RegisterStats( )
{

		AddStat( miss_count );
		AddStat( hit_count  );
		AddStat( evict_count );
		AddStat( wb_count    );
		AddStat( fill_count );
		AddStat( write_hit );
		AddStat( read_hit);
		AddStat( hit_rate);
		AddStat( read_miss);
		AddStat( write_miss);
		MemoryController::RegisterStats();
}


/*
 *
 */
void WayPredictDRC::CalculateStats()
{
	if( hit_count + miss_count > 0)
		hit_rate = (double)hit_count/(double)(hit_count+miss_count);

	MemoryController::CalculateStats();
}
