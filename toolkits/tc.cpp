/*
Copyright (c) 2014-2015 Xiaowei Zhu, Tsinghua University

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <chrono>
#include <iostream>
#include <mutex>

#include "core/type.hpp"
#include "core/graph.hpp"


using namespace std::chrono;

template<typename T>
class SafeVector {
private:
    std::vector<T> data;
    mutable std::mutex mutex_;

public:
    void push_back(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data.push_back(value);
    }

    std::vector<T>& get() {
        return data;
    }
    const std::vector<T>& get() const {
        return data;
    }
};

void compute(Graph<Empty> * graph, EdgeId& total_triangles, EdgeId& visited_subgraphs) {
  VertexSubset * active = graph->alloc_vertex_subset();
  active->fill();

  std::vector<SafeVector<VertexId>> adjs(graph->vertices);

  graph->process_edges<VertexId,VertexAdjList<Empty>>(
    [&](VertexId src){
      // PASS
    },
    [&](VertexId src, VertexAdjList<Empty> msg, VertexAdjList<Empty> outgoing_adj){
      // PASS
      return 0;
    },
    [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
      graph->emit(dst, incoming_adj);
    },
    [&](VertexId dst, VertexAdjList<Empty> msg) {
      // printf("%u,%p,%p\n", dst, msg.begin, msg.end);
      // adjs[dst] = std::vector<VertexId>{};
      for (AdjUnit<Empty>* ptr = msg.begin; ptr != msg.end; ptr++) {
        VertexId src = ptr->neighbour;
        adjs[dst].push_back(src);
      }
      return 0;
    },
    active
  );

  graph->process_vertices<int>(
    [&](VertexId vtx) {
      std::sort(adjs[vtx].get().begin(), adjs[vtx].get().end());
      return 0;
    },
    active
  );

  auto find_triangles = [&](const std::vector<VertexId>* src_list, const std::vector<VertexId>* dst_list,
                            VertexId src, VertexId dst, EdgeId* visited_subgraphs) -> EdgeId {

    EdgeId src_len = src_list->size();
    EdgeId dst_len = dst_list->size();
    if (src_len == 0 || dst_len == 0) {
      return 0;
    }
    if (src_len > dst_len) {
      std::swap(src_len, dst_len);
      std::swap(src_list, dst_list);
    }

    VertexId needle = dst_list->front();
    auto src_search_start = std::lower_bound(src_list->begin(), src_list->end(), needle);

    if (src_search_start == src_list->end()) {
      return 0;
    }
    auto dst_search_start = dst_list->begin();
    
    EdgeId intersection_count = 0;
    EdgeId src_visited = src_search_start - src_list->begin();
    EdgeId dst_visited = dst_search_start - dst_list->begin();

    while (src_search_start != src_list->end() &&
           dst_search_start != dst_list->end()) {
      auto cur_src = *src_search_start;
      auto cur_dst = *dst_search_start;
      if (cur_src == cur_dst) {
        ++src_search_start;
        ++dst_search_start;
        ++intersection_count;
        // printf("find a triangle: %d %d %d\n", src, dst, cur_src);
      } else if (cur_src < cur_dst) {
        ++src_search_start;
      } else {
        ++dst_search_start;
      }
    }

    src_visited = (src_search_start - src_list->begin()) - src_visited;
    dst_visited = (dst_search_start - dst_list->begin()) - dst_visited;
    write_add(visited_subgraphs, src_visited + dst_visited);

    return intersection_count;
  };

  EdgeId vis_subgraphs = 0;

  EdgeId result = graph->process_edges<EdgeId, EdgeId>(
    [&](VertexId src){
      // PASS
    },
    [&](VertexId src, double msg, VertexAdjList<Empty> outgoing_adj){
      // PASS
      return 0;
    },
    [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
      const std::vector<VertexId>& dst_adj_vec = adjs[dst].get();
      // std::vector<VertexId> dst_adj_vec = make_adj_vector(dst_adj);
      EdgeId traingle_cnt = 0;
      for (AdjUnit<Empty>* ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++) {
        VertexId src = ptr->neighbour;
        if (src >= dst) continue;

        const std::vector<VertexId>& src_adj_vec = adjs[src].get();
        traingle_cnt += find_triangles(&src_adj_vec, &dst_adj_vec, src, dst, &vis_subgraphs);
      }
      graph->emit(dst, traingle_cnt);
    },
    [&](VertexId dst, EdgeId msg) {
      return msg;
    },
    active
  );
  
  // printf("Triangle count: %lu\n", result);

  delete active;

  total_triangles = result;
  visited_subgraphs = vis_subgraphs;
}

int main(int argc, char** argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc < 3) {
    printf("tc [file] [vertices] (n_runs)\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  graph->load_undirected_from_directed(argv[1], std::atoi(argv[2]));

  // compute(graph);
  EdgeId total_triangles = 0;
  EdgeId visited_subgraphs = 0;
  auto t_start = high_resolution_clock::now();
  int n_valid = argc >= 4 ? std::atoi(argv[3]) : 1;
  assert(n_valid > 0);
  
  for (int run = 0; run < n_valid; run++) {
    compute(graph, total_triangles, visited_subgraphs);
  }
  auto t_stop = high_resolution_clock::now();
  auto elapsed = duration_cast<microseconds>(t_stop - t_start).count();

  float avg_time = (float)(elapsed) / 1000 / n_valid;
  std::cout << "Valid Runs : " << n_valid << "\n";
  std::cout << "Total Triangles : " << total_triangles << std::endl;
  std::cout << "Average Elapsed Time : " << avg_time << " (ms)"
            << std::endl;
  std::cout << "Visited Subgraphs: " << visited_subgraphs << "\n";
  std::cout << "GTSPS : " << (visited_subgraphs / 1e9) / (avg_time / 1000)
            << std::endl;
  
  delete graph;
  return 0;
}
