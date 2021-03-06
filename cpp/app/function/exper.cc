#include <gflags/gflags.h>
#include <glog/logging.h>
#include<boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include "cpp/core/graphapi.h"
#include "cpp/serial/dualsimulation.h"
#include "cpp/serial/dual_incremental.h"
#include "cpp/serial/strongsimulation.h"
#include "cpp/serial/strong_incremental.h"
#include "cpp/utils/util.h"
#include "cpp/io/io_local.h"
#include "cpp/core/global.h"
#include "cpp/core/strongr.h"
#include "cpp/utils/generate.h"
#include "cpp/utils/util.h"
#include<iostream>
#include <fstream>
#include<ctime>
//#include <sstream>
// #define random(a,b) (rand()%(b-a+1)+a)
  
class ExperExr{
public:

    ExperExr(){}

    ExperExr(std::string test_data_name,int query_index){
        this->test_data_name=test_data_name;
        this->graph_vfile ="../data/"+test_data_name+"/"+test_data_name+".v";
        this->graph_efile ="../data/"+test_data_name+"/"+test_data_name+".e";
        this->r_file = "../data/"+test_data_name+"/"+test_data_name+".r";
        this->base_qfile = "../data/"+test_data_name+"/query4_6_2/q";
        this->base_add_file = "../data/"+test_data_name+"/inc/add_e";
        this->base_remove_file="../data/"+test_data_name+"/inc/rm_e";
        this->base_add_affected_center_file ="../data/"+test_data_name+"/inc/affectedcenter_adde.txt";
        this->base_remove_affected_center_file ="../data/"+test_data_name+"/inc/affectedcenter_rme.txt";
        this->query_index = query_index;
    }
    std::string get_query_vfile(int index){
        return base_qfile+std::to_string(index)+".v";
    }

    std::string get_query_efile(int index){
        return base_qfile+std::to_string(index)+".e";
    }
public:
  void print_graph_info(Graph &graph){
      std::cout<<"dgraph vertices nums: "<<graph.GetNumVertices()<<" dgraph edgee nums: "<<graph.GetNumEdges()<<endl;
      for(auto u:graph.GetAllVerticesID()){
          cout<<"id->label:"<<u<<' '<<graph.GetVertexLabel(u)<<endl;
      }
      for(auto e:graph.GetAllEdges()){
          cout<<"edges:"<<e.src()<<' '<<e.dst()<<endl;
      }
  }

  void generate_query_base_dgraph(int generate_query_nodes, int max_calculate_center_nodes,int generate_query_nums){
      Graph dgraph;
      Generate generate;
      GraphLoader dgraph_loader;
      dgraph_loader.LoadGraph(dgraph,graph_vfile,graph_efile);
      std::cout<<dgraph.GetNumVertices()<<' '<<dgraph.GetNumEdges()<<std::endl;
      int i=1;
      std::fstream tmp_outfile("../data/dbpedia/query6_8_3/query_info.txt",std::ios::out);
      tmp_outfile.close();
      while(i<=generate_query_nums){
          Graph qgraph;
          generate.generate_connect_graphs_by_Dgraph(dgraph,qgraph,generate_query_nodes);
          int d_Q=cal_diameter_qgraph(qgraph);
          if((d_Q!=3) || (qgraph.GetNumEdges()!=8) || !query_labl_all_notsame(qgraph)){
              continue;
          }
          clock_t s0,e0;
          s0 =clock();
          std::unordered_set<VertexID> max_dual_set = generate.get_dual_node_result(dgraph,qgraph);
          e0 =clock();
          if(max_dual_set.size()<=max_calculate_center_nodes){
              generate.save_grape_file(qgraph,get_query_vfile(i),get_query_efile(i));
              std::cout<<i<<' '<<"calculate dual time"<<(float)(e0-s0)/CLOCKS_PER_SEC<<"s"<<' '<<max_dual_set.size()<<std::endl;
              std::fstream tmp_outfile("../data/dbpedia/query6_8_3/query_info.txt",std::ios::app);
              tmp_outfile<<i<<' '<<"calculate dual time"<<(float)(e0-s0)/CLOCKS_PER_SEC<<"s"<<' '<<max_dual_set.size()<<std::endl;
              tmp_outfile.close();
              i++;
          }
      }
  }

  void generate_query_random(int query_nodes_nums,int query_edges_nums,int d_Q,int max_calculate_center_nodes,int verex_label_num,int generate_query_nums){
     Graph dgraph;
     Generate generate;
     GraphLoader dgraph_loader;
     dgraph_loader.LoadGraph(dgraph,graph_vfile,graph_efile);
     std::vector<VertexLabel> labels =generate.get_graph_label_vec(dgraph);
     std::cout<<"dgraph vertices nums: "<<dgraph.GetNumVertices()<<" dgraph edgee nums: "<<dgraph.GetNumEdges()<<" dgraph label nums: "<<labels.size()<<std::endl;
     int i=1;
     while(i<=generate_query_nums){
          Graph qgraph;
          if(verex_label_num==0){
              generate.generate_random_connectivity_graph(qgraph,query_nodes_nums,query_edges_nums,labels);
          }else{
              generate.generate_random_connectivity_graph(qgraph,query_nodes_nums,query_edges_nums,verex_label_num);
          }
          int d_q=cal_diameter_qgraph(qgraph);
          if(d_q>d_Q || !query_labl_all_notsame(qgraph)){
              continue;
          }
          clock_t s0,e0;
          s0 =clock();
          std::unordered_set<VertexID> max_dual_set = generate.get_dual_node_result(dgraph,qgraph);
          e0 =clock();
          if(max_dual_set.size()<=max_calculate_center_nodes){
              generate.save_grape_file(qgraph,get_query_vfile(i),get_query_efile(i));
             // print_graph_info(qgraph);
              std::cout<<i<<' '<<"calculate dual time"<<(float)(e0-s0)/CLOCKS_PER_SEC<<"s"<<' '<<max_dual_set.size()<<std::endl;
              i++;
          }
     }
}

  std::unordered_set<VertexID> find_affected_area(Graph &dgraph,std::set<std::pair<VertexID,VertexID>> &add_edges,int d_Q){
     std::unordered_set<VertexID> affected_nodes,changenode;
     for(auto e:add_edges){
        changenode.insert(e.first);
        changenode.insert(e.second);
     }
     dgraph.find_hop_nodes(changenode,d_Q,affected_nodes);
     return affected_nodes;
  }

  void print_evaluate_incremental_information(int circle_num){
        Graph dgraph;
        GraphLoader dgraph_loader,qgraph_loader;
        dgraph_loader.LoadGraph(dgraph,graph_vfile,graph_efile);
        Graph qgraph;
        qgraph_loader.LoadGraph(qgraph,get_query_vfile(query_index),get_query_efile(query_index));
        int d_Q = cal_diameter_qgraph(qgraph);
        std::unordered_map<VertexID, std::unordered_set<VertexID>> sim;
        DualInc dualinc;
        DualSim dualsim;
        bool initialized_sim=false;
        dualsim.dual_simulation(dgraph,qgraph,sim,initialized_sim);
        std::unordered_set<int> max_dual_set;
        for(auto u :qgraph.GetAllVerticesID()){
            for(auto v:sim[u]){
               max_dual_set.insert(v);
            }
        }
        std::cout<<"original need calculate center node "<<max_dual_set.size()<<endl;
        int j=1;
        while(j<circle_num){
            std::set<std::pair<VertexID,VertexID>> add_edges,rm_edges;
            std::unordered_map<VertexID, std::unordered_set<VertexID>> incdsim;
            for(auto u :qgraph.GetAllVerticesID()){
                incdsim[u]=std::unordered_set<VertexID>();
                for(auto v:sim[u]){
                incdsim[u].insert(v);
                }
            }
            Load_bunch_edges(add_edges,base_add_file,j);
            Load_bunch_edges(rm_edges,base_remove_file,j);
            std::set<pair<VertexID,VertexID>> tmp_set;
            for(auto e:add_edges){
                tmp_set.insert(e);
            }
            for(auto e:rm_edges){
               tmp_set.insert(e);
            }
            std::unordered_set<VertexID> e_affected_nodes = find_affected_area(dgraph,tmp_set,d_Q);
            e_affected_nodes = intersection(e_affected_nodes,max_dual_set);
          //  LoadEdges(edges,base_add_file+std::to_string(j));
        //    LoadEdges(edges,base_remove_file+std::to_string(j));
            for (auto e:add_edges){
               dgraph.AddEdge(Edge(e.first,e.second,1));
            }
            dualinc.incremental_addedges(dgraph,qgraph,incdsim,add_edges);
            for(auto e :rm_edges){
                dgraph.RemoveEdge(Edge(e.first,e.second,1));
            }
            dualinc.incremental_removeedgs(dgraph,qgraph,incdsim,rm_edges);
            std::unordered_set<VertexID> inc_max_dual_set;
            for(auto u :qgraph.GetAllVerticesID()){
                for(auto v:incdsim[u]){
                   inc_max_dual_set.insert(v);
                }
            }
            for(auto e :rm_edges){
                dgraph.AddEdge(Edge(e.first,e.second,1));
            }
            for(auto e:add_edges){
                dgraph.RemoveEdge(Edge(e.first,e.second,1));
            }
            std::cout<<j<<' '<<"after incremental edgs all need calculate center nodes: "<<inc_max_dual_set.size()<<' '<<"      incremental edges affected center node nums: "<<e_affected_nodes.size()<<endl;
            j+=1;
        }
  }

  void save_edges_app(std::set<std::pair<VertexID,VertexID>> &edges, const std::string efile){
  //std::cout<<efile<<std::endl;
   std::fstream outfile(efile,std::ios::app);
   if(!outfile)
	{
		std::cout<<"the outfile can  not construct";
		exit(0);
	}
	for(auto e:edges){
	 outfile<<e.first<<' '<<e.second<<std::endl;
	}
	outfile.close();
}


void generate_all_random_edges(int num_edges,int circle_num,int new_vertices_rate){
    Graph dgraph;
    GraphLoader dgraph_loader;
    dgraph_loader.LoadGraph(dgraph,graph_vfile,graph_efile);
    int dgraph_num_vertices = dgraph.GetNumVertices();
    std::set<pair<VertexID,VertexID>> exist_edges;
    LoadEdges(exist_edges,graph_efile);
    cout<<"Base Graph vertices: "<<dgraph_num_vertices<<"  Base Graph edges: "<<dgraph.GetNumEdges()<<endl;
    int i=1;
    while(i<=circle_num){
        // Graph dgraph;
        // load_graph(dgraph,graph_vfile,graph_efile, base_add_file,base_remove_file,exist_edges,i-1);
        std::set<std::pair<VertexID,VertexID>> add_e_set,remove_e_set;

		generate_all_random_remove_edges(remove_e_set, exist_edges, num_edges);
        for(auto e:remove_e_set){
            exist_edges.erase(e);
        }
        save_edges(remove_e_set,base_remove_file+std::to_string(i));
	    cout<<"after remove "<<remove_e_set.size()<<" egdes, dgraph_num_vertices: "<<dgraph_num_vertices<<"  exist_edges: "<<exist_edges.size()<<endl;
		
	    generate_all_random_add_edge_include_new_vertices(add_e_set,exist_edges,dgraph_num_vertices,num_edges,new_vertices_rate);
        for(auto e : add_e_set){
            exist_edges.insert(e);
        }
        save_edges(add_e_set,base_add_file+std::to_string(i));
	    cout<<"after add "<<add_e_set.size()<<" egdes, dgraph_num_vertices: "<<dgraph_num_vertices<<"  exist_edges: "<<exist_edges.size()<<endl;
        
        std::cout<<i<<' '<<add_e_set.size()<<' '<<remove_e_set.size()<<std::endl;
        add_e_set.clear();
        remove_e_set.clear();
        i++;
    }
	// std::cout<<"after add edges , the new added vertices num = "<< dgraph_num_vertices - dgraph.GetNumVertices()<<std::endl;
}


std::set<std::pair<VertexID,VertexID>> generate_all_random_add_edges(std::set<std::pair<VertexID,VertexID>> &exist_edges,int dgraph_num_vertices,int num_edges){
   srand( (unsigned)time(0));
    int j=0;
    std::set<std::pair<VertexID,VertexID>> add_edges;
    while(j<num_edges){
         VertexID node1 = random(0,dgraph_num_vertices-1);
         VertexID node2 = random(0,dgraph_num_vertices-1);
         if (node1!=node2){
             //std::pair<VertexID,VertexID> p(node1,node2);
             if (exist_edges.find(std::make_pair(node1,node2)) == exist_edges.end() && add_edges.find(std::make_pair(node1,node2)) == add_edges.end()){
                 add_edges.insert(std::make_pair(node1,node2));
                 j+=1;
             }
          }
    }
   return add_edges;
}

//------------------------------------------------------------
void generate_all_random_add_edge_include_new_vertices(std::set<std::pair<VertexID,VertexID>> &add_edges, 
											std::set<std::pair<VertexID,VertexID>> &exist_edges, int dgraph_num_vertices,
											int add_size, int new_vertices_rate){ // add_size includes vertices and edges.
	srand((unsigned)time(0));
	int j = 0;
	int tmp;
	int new_whole_vnum = dgraph_num_vertices + dgraph_num_vertices*new_vertices_rate/100;
	std::random_device rd;
    std::mt19937 gen(rd());
	std::discrete_distribution<int> chance_old_or_new({50, 12, 13, 25}); // the chance of [0,1,2,3] are [50%,12%,13%,25%] respectively.
    while(j<add_size){
		tmp = chance_old_or_new(gen);
		if(tmp == 0){        // both old
		    VertexID node1 = random(0, dgraph_num_vertices-1);
			VertexID node2 = random(0, dgraph_num_vertices-1);
			while(node2 == node1){
				node2 = random(0, dgraph_num_vertices-1);
			}
			if (exist_edges.find(std::make_pair(node1,node2)) == exist_edges.end() && add_edges.find(std::make_pair(node1,node2)) == add_edges.end()){
				add_edges.insert(std::make_pair(node1,node2));
				j += 1;
			}
		}
		else if(tmp == 1) {  // half new, source is old.
			VertexID node1 = random(0, dgraph_num_vertices-1);
			VertexID node2 = random(dgraph_num_vertices, new_whole_vnum);
			add_edges.insert(std::make_pair(node1,node2));
			j += 2;
		}
		else if(tmp == 2){   // half new, target is old.
			VertexID node1 = random(dgraph_num_vertices, new_whole_vnum);
			VertexID node2 = random(0, dgraph_num_vertices-1);
			add_edges.insert(std::make_pair(node1,node2));
			j += 2;
		}
		else{                // both new
			VertexID node1 = random(dgraph_num_vertices, new_whole_vnum);
			VertexID node2 = random(dgraph_num_vertices, new_whole_vnum);
			while(node2 == node1){
				node2 = random(dgraph_num_vertices, new_whole_vnum);
			}
			add_edges.insert(std::make_pair(node1,node2));
			j += 3;
		}
    }
}
//------------------------------------------------------------

void generate_all_random_remove_edges(std::set<std::pair<VertexID,VertexID>>  &remove_e_set,
								std::set<std::pair<VertexID,VertexID>> &exist_edges, int num_edges){
    srand((unsigned)time(0));
    std::vector<std::pair<VertexID,VertexID>> edge_list;
    for(auto e :exist_edges){
        edge_list.push_back(e);
    }
    std::unordered_set<int> record;
    int j=0;
	int n1;
    while(j<num_edges){
          n1 = random(0, edge_list.size()-1);
          if(record.find(n1) == record.end()){
              remove_e_set.insert(edge_list[n1]);
              record.insert(n1);
              j++;
          }
    }
}

void calculate_direct_strong_inc(Graph &dgraph,Graph &qgraph,
                                      std::set<std::pair<VertexID,VertexID>> &add_edges,
                                      std::set<std::pair<VertexID,VertexID>> &rm_edges,
									  std::vector<StrongR> &result,
									  int flag){
    if(flag == 2){ //first remove, then add.
		StrongSim strongsim;
		clock_t stime1 = clock();
		for(auto e : rm_edges){
			dgraph.RemoveEdge(Edge(e.first,e.second,1));
		}
		dgraph.RebuildGraphProperties();
		// dgraph.printGraphInfo();
		
		int max = dgraph.GetNumVertices() - 1;
		for(auto e : add_edges){
			if(e.first > max){
				max = e.first;
			}
			if(e.second > max){
				max = e.second;
			}
		}
		int tmp = dgraph.GetNumVertices();
		for(int i=0; i<=(max-tmp); i++){
			dgraph.AddVertex(Vertex(i, (i+123456789)%MAX_LABEL)); // save it.
		}		
		for(auto e : add_edges){
			dgraph.AddEdge(Edge(e.first,e.second,1));
		}
		dgraph.RebuildGraphProperties();		
		// dgraph.printGraphInfo();
		clock_t etime1 = clock();	

		result = strongsim.strong_simulation_sim(dgraph,qgraph,2);

		std::fstream outfile3("time_info_rm_and_add.txt",std::ios::app);
		outfile3<<(float)(etime1-stime1)/CLOCKS_PER_SEC<<" ";
		outfile3.close();
		// std::cout<<"Direct..........After add and rm edges : "<<std::endl;
	}
	else if(flag == 0){ // only remove.
		StrongSim strongsim;
		clock_t stime1 = clock();
		for(auto e : rm_edges){
			dgraph.RemoveEdge(Edge(e.first,e.second,1));
		}
		dgraph.RebuildGraphProperties();
		// dgraph.printGraphInfo();
		clock_t etime1 = clock();	
		result = strongsim.strong_simulation_sim(dgraph,qgraph,0);
		std::fstream outfile3("time_info_only_rm.txt",std::ios::app);
		outfile3<<(float)(etime1-stime1)/CLOCKS_PER_SEC<<" ";
		outfile3.close();	
	}
	else if(flag == 1){ // only add.
		StrongSim strongsim;
		clock_t stime1 = clock();
		int max = dgraph.GetNumVertices() - 1;
		for(auto e : add_edges){
			if(e.first > max){
				max = e.first;
			}
			if(e.second > max){
				max = e.second;
			}
		}
		int tmp = dgraph.GetNumVertices();
		for(int i=0; i<=(max-tmp); i++){
			dgraph.AddVertex(Vertex(i, (i+123456789)%MAX_LABEL)); // save it.
		}
		for(auto e : add_edges){
			dgraph.AddEdge(Edge(e.first,e.second,1));
		}
		dgraph.RebuildGraphProperties();		
		// dgraph.printGraphInfo();
		clock_t etime1 = clock();	

		result = strongsim.strong_simulation_sim(dgraph,qgraph,1);

		std::fstream outfile3("time_info_only_add.txt",std::ios::app);
		outfile3<<(float)(etime1-stime1)/CLOCKS_PER_SEC<<" ";
		outfile3.close();
	}
}

void first_strong_strongInc(int circle_num, int flag){
    StrongInc stronginc;
    StrongSim strongsim;
    DualSim dualsim;

    Graph dgraph;
    GraphLoader dgraph_loader,qgraph_loader;
    dgraph_loader.LoadGraph(dgraph,graph_vfile,graph_efile);
    int index =query_index;
    Generate generate;
    Graph qgraph;
    qgraph_loader.LoadGraph(qgraph,get_query_vfile(index),get_query_efile(index));
    std::unordered_map<VertexID, std::unordered_set<VertexID>> sim;
    clock_t s0 = clock();
    std::vector<StrongR> strongsimr = strongsim.strong_simulation_sim(dgraph,qgraph);
    clock_t e0 = clock();
    std::cout<<"calculate original strong"<<(float)(e0-s0)/CLOCKS_PER_SEC<<"s"<<std::endl;
	  
    bool initialized_sim = false;
    dualsim.dual_simulation(dgraph,qgraph,sim,initialized_sim);	  
    std::unordered_set<VertexID> max_dual_set = generate.get_dual_node_result(dgraph,qgraph);
    std::cout<<"max dual size "<<max_dual_set.size()<<std::endl;
	   
    int j = 1;
	
	if(flag == 2){
		std::fstream outfile("runtime_rm_and_add.txt",std::ios::out);
		outfile.close();
		std::fstream outfile1("time_info_rm_and_add.txt",std::ios::out);
		outfile1.close();
		std::fstream outfile2("whole_time_info_rm_and_add.txt",std::ios::out);
		outfile2.close();
		std::fstream o_tmp1("check_strongr_results.txt",std::ios::app);
		o_tmp1<<"Add and Remove : \n";
		o_tmp1.close();
	}
	else if(flag == 0){
		std::fstream outfile("runtime_only_rm.txt",std::ios::out);
		outfile.close();
		std::fstream outfile1("time_info_only_rm.txt",std::ios::out);
		outfile1.close();
		std::fstream outfile2("whole_time_info_only_rm.txt",std::ios::out);
		outfile2.close();
		std::fstream o_tmp1("check_strongr_results.txt",std::ios::app);
		o_tmp1<<"Only Remove : \n";
		o_tmp1.close();
	}
	else if(flag == 1){
		std::fstream outfile("runtime_only_add.txt",std::ios::out);
		outfile.close();
		std::fstream outfile1("time_info_only_add.txt",std::ios::out);
		outfile1.close();
		std::fstream outfile2("whole_time_info_only_add.txt",std::ios::out);
		outfile2.close();
		std::fstream o_tmp1("check_strongr_results.txt",std::ios::app);
		o_tmp1<<"Only Add : \n";
		o_tmp1.close();
	}
	
    while (j<=circle_num){
        GraphLoader dgraph_loaddir,dgraph_loadinc;
        Graph dgraphdir,dgraphinc;
	    dgraph_loaddir.LoadGraph(dgraphdir,graph_vfile,graph_efile);
	    dgraph_loadinc.LoadGraph(dgraphinc,graph_vfile,graph_efile);
		  
        std::set<std::pair<VertexID,VertexID>> add_edges,rm_edges;
        // dgraphdir.printGraphInfo();
        // dgraphinc.printGraphInfo();
		if(flag == 2){
			Load_bunch_edges(rm_edges,base_remove_file,j);
			Load_bunch_edges(add_edges,base_add_file,j);
		}
		else if(flag == 0){
			Load_bunch_edges(rm_edges,base_remove_file,j);
		}
		else if(flag == 1){
			Load_bunch_edges(add_edges,base_add_file,j);
		}
        std::vector<StrongR> tmp_r;
        std::unordered_map<VertexID, std::unordered_set<VertexID>> tmp_sim;
        for(auto ball:strongsimr){
            tmp_r.push_back(ball);
        }
        for(auto u :qgraph.GetAllVerticesID()){
            tmp_sim[u]=std::unordered_set<VertexID>();
            for(auto v:sim[u]){
                tmp_sim[u].insert(v);
            }
        }
		  
        std::vector<StrongR> direct_strong;
		clock_t start1 = clock();
		calculate_direct_strong_inc(dgraphdir,qgraph,add_edges,rm_edges,direct_strong,flag);
        clock_t end1 = clock();
        // std::cout<<"calculate direct strong"<<(float)(end1-start1)/CLOCKS_PER_SEC<<"s"<<std::endl;
		
        clock_t start2 = clock();
        stronginc.strong_simulation_inc(dgraphinc,qgraph,tmp_sim,tmp_r,add_edges,rm_edges,flag);
        clock_t end2 = clock();
        //std::cout<<"calculate inc strong"<<(float)(end2-start2)/CLOCKS_PER_SEC<<"s"<<std::endl;
 	      
    	std::fstream out_tmp("check_strongr_results.txt",std::ios::app);
        out_tmp<<j<<" direct strong results: "<<direct_strong.size()<<" ;"<<" inc strong results: "<<tmp_r.size()<<"\n";
        out_tmp.close();		
		std::cout<<j<<" direct strong results: "<<direct_strong.size()<<" ;"<<" inc strong results: "<<tmp_r.size()<<"\n";
		/*
		std::cout<<"direct strong results:\n";
		for(auto ball : direct_strong){
			std::cout<<"center = "<<ball.center()<<std::endl;
			std::unordered_map<VertexID,std::unordered_set<VertexID>> sim_ = ball.ballr();
			for(auto u : sim_){
				std::cout<<u.first<<" -> ";
				for(auto v : sim_[u.first])
					std::cout<<v<<" ";
				std::cout<<std::endl;
			}
			std::cout<<"-------------------"<<std::endl;
		}
		std::cout<<"---------------------------------------------------------------------------\ninc strong results:\n";
		for(auto ball : tmp_r){
			std::cout<<"center = "<<ball.center()<<std::endl;
			std::unordered_map<VertexID,std::unordered_set<VertexID>> sim_ = ball.ballr();
			for(auto u : sim_){
				std::cout<<u.first<<" -> ";
				for(auto v : sim_[u.first])
					std::cout<<v<<" ";
				std::cout<<std::endl;
			}
			std::cout<<"-------------------"<<std::endl;
		}
		*/		
        cout<<(float)(end1-start1)/CLOCKS_PER_SEC<<' '<<(float)(end2-start2)/CLOCKS_PER_SEC<<endl;
		
		if(flag == 2){
			std::fstream outfile("runtime_rm_and_add.txt",std::ios::app);
			outfile<<j<<' '<<(float)(end1-start1)/CLOCKS_PER_SEC<<' '<<(float)(end2-start2)/CLOCKS_PER_SEC<<endl;
			outfile.close();
		}
		else if(flag == 0){
			std::fstream outfile("runtime_only_rm.txt",std::ios::app);
			outfile<<j<<' '<<(float)(end1-start1)/CLOCKS_PER_SEC<<' '<<(float)(end2-start2)/CLOCKS_PER_SEC<<endl;
			outfile.close();
		}
		else if(flag == 1){
			std::fstream outfile("runtime_only_add.txt",std::ios::app);
			outfile<<j<<' '<<(float)(end1-start1)/CLOCKS_PER_SEC<<' '<<(float)(end2-start2)/CLOCKS_PER_SEC<<endl;
			outfile.close();
		}
		
        j += 1;
    }
	std::fstream o_tmp("check_strongr_results.txt",std::ios::app);
	o_tmp<<"------------------------------------------------------\n";
	o_tmp.close();
}

private:
    std::string test_data_name ="yago";
    std::string graph_vfile ="../data/yago/yago.v";
    std::string graph_efile ="../data/yago/yago.e";
    std::string r_file = "../data/yago/yago.r";
    std::string base_qfile = "../data/yago/query/q";
    std::string base_add_file = "../data/yago/inc/add_e";
    std::string base_remove_file="../data/yago/inc/rm_e";
    std::string base_add_affected_center_file ="../data/yago/inc/affectedcenter_adde.txt";
    std::string base_remove_affected_center_file ="../data/yago/inc/affectedcenter_rme.txt";
    int query_index = 1;
};


int main(int argc, char *argv[]) {
  google::SetUsageMessage("Usage: test [gflags_opt]");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::ShutDownCommandLineFlags();
  google::InitGoogleLogging("test for working");
  google::ShutdownGoogleLogging();
  init_workers();

  std::fstream outfile1("calculate_ball_info.txt",std::ios::out);//记录direct strong sim时计算每个ball的时间
  outfile1.close();  
  std::fstream outfile2("calculate_ball_info_inc.txt",std::ios::out);//记录inc strong sim时计算每个ball的时间
  outfile2.close();
  std::fstream outfile3("check_strongr_results.txt",std::ios::out); //比较direct和inc得到的结果数量，看结果是否正确
  outfile3.close();
  
  string base_name="dbpedia";
  ExperExr experExr(base_name,31);
  // ExperExr experExr(base_name,2);
  
//  experExr.generate_query_base_dgraph(6,7000,500);  // 生成查询图Q。   (点数，生成最大ball数，生成Q的数量)
//  experExr.generate_all_random_edges(447944,18,50); // 完全随机生成增减边。 （每次增加或者减少的量，增加和减少的次数，新增的点所占比例） 
//  experExr.generate_all_random_edges(3,5,50);    // test.
						  // dbpedia: 4639253+13278535=17917788, 17917788*2.5%=447944.7, 相当于每次增加2.5%  再减少2.5%，共5%
//  experExr.generate_query_random(5,10,2,10000,0,5);

// The first parameter is the number of iterations, 
// The second parameter is [0,1,2], represents "only add", "only remove", "first remove, then add" respectively.
  experExr.first_strong_strongInc(18,0);  // only remove.
  experExr.first_strong_strongInc(18,1);  // only add.
  experExr.first_strong_strongInc(18,2);  // first remove, then add.

  // test.
  // experExr.first_strong_strongInc(3,0);  // only remove.
  // experExr.first_strong_strongInc(3,1);  // only add.
  // experExr.first_strong_strongInc(3,2);  // first remove, then add.
  
  worker_finalize();
  return 0;
}

using namespace std;
