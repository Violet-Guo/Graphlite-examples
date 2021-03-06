/**
 * @file PageRankVertex.cc
 * @author  Songjie Niu, Shimin Chen
 * @version 0.1
 *
 * @section LICENSE 
 * 
 * Copyright 2016 Shimin Chen (chensm@ict.ac.cn) and
 * Songjie Niu (niusongjie@ict.ac.cn)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @section DESCRIPTION
 * 
 * This file implements the PageRank algorithm using graphlite API.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "GraphLite.h"

#define VERTEX_CLASS_NAME(name) Color##name

#define EPS 1e-6
#define random(x)(rand()%x)
int color_num;
int64_t v0id;

typedef struct{

    int64_t vid;
    int mycolor;
    int neighbor_size;
    int neighbors[20];

}VertexC;


class VERTEX_CLASS_NAME(InputFormatter): public InputFormatter {
public:
    int64_t getVertexNum() {
        unsigned long long n;
        sscanf(m_ptotal_vertex_line, "%lld", &n);
        m_total_vertex= n;
        return m_total_vertex;
    }
    int64_t getEdgeNum() {
        unsigned long long n;
        sscanf(m_ptotal_edge_line, "%lld", &n);
        m_total_edge= n;
        return m_total_edge;
    }
    int getVertexValueSize() {
        m_n_value_size = sizeof(double);
        return m_n_value_size;
    }
    int getEdgeValueSize() {
        m_e_value_size = sizeof(double);
        return m_e_value_size;
    }
    int getMessageValueSize() {
        m_m_value_size = sizeof(int);
        return m_m_value_size;
    }
    void loadGraph() {
        unsigned long long last_vertex;
        unsigned long long from;
        unsigned long long to;
        double weight = 0;
        
        double value = 1;
        int outdegree = 0;
        
        const char *line= getEdgeLine();

        // Note: modify this if an edge weight is to be read
        //       modify the 'weight' variable

        sscanf(line, "%lld %lld", &from, &to);
        addEdge(from, to, &weight);

        last_vertex = from;
        ++outdegree;
        for (int64_t i = 1; i < m_total_edge; ++i) {
            line= getEdgeLine();

            // Note: modify this if an edge weight is to be read
            //       modify the 'weight' variable

            sscanf(line, "%lld %lld", &from, &to);
            if (last_vertex != from) {
                addVertex(last_vertex, &value, outdegree);
                last_vertex = from;
                outdegree = 1;
            } else {
                ++outdegree;
            }
            addEdge(from, to, &weight);
        }
        addVertex(last_vertex, &value, outdegree);
    }
};

class VERTEX_CLASS_NAME(OutputFormatter): public OutputFormatter {
public:
    void writeResult() {
        int64_t vid;
        double value;
        char s[1024];

        for (ResultIterator r_iter; ! r_iter.done(); r_iter.next() ) {
            r_iter.getIdValue(vid, &value);
            int n = sprintf(s, "%lld: %f\n", (unsigned long long)vid, value);
            writeNextResLine(s, n);
        }
    }
};

// An aggregator that records a double value tom compute sum
class VERTEX_CLASS_NAME(Aggregator): public Aggregator<double> {
public:
    void init() {
        m_global = 0;
        m_local = 0;
    }
    void* getGlobal() {
        return &m_global;
    }
    void setGlobal(const void* p) {
        m_global = * (double *)p;
    }
    void* getLocal() {
        return &m_local;
    }
    void merge(const void* p) {
        m_global += * (double *)p;
    }
    void accumulate(const void* p) {
        m_local += * (double *)p;
    }
};


class VERTEX_CLASS_NAME(): public Vertex <VertexC, double, int> {
public:
    void compute(MessageIterator* pmsgs) {
        VertexC vertexValue;
       
        if (getSuperstep() == 0) {
            vertexValue.mycolor = -1;
            vertexValue.vid = getVertexId();
            vertexValue.neighbor_size = 0;
            if(getVertexId() == v0id){
                vertexValue.mycolor = 0;
            }
            * mutableValue() = vertexValue;
            sendMessageToAllNeighbors(vertexValue.mycolor);
            voteToHalt(); 

        } else {
            // if (getSuperstep() >= 2) {
            //     double global_val = * (double *)getAggrGlobal(0);
            //     if (global_val < EPS) {
            //         voteToHalt(); return;
            //     }
            // }
            int tmp[20] = {0}; // store unused color number
            int available; //available colors num
            vertexValue.mycolor = getValue().mycolor;
            int i = 0;
            for ( ; ! pmsgs->done(); pmsgs->next() ) {
                vertexValue.neighbors[i] = pmsgs->getValue();
                i++;
            }
            vertexValue.neighbor_size = i;


            if(vertexValue.mycolor == -1){
                int p = 0;
                for(int i = 0; i < color_num; i++){
                    for(int j = 0; j < vertexValue.neighbor_size; j++){
                        if(i != vertexValue.neighbors[j]){
                            tmp[p] = i;
                            p++;
                        }else continue;
                    }
                }

                int t = random(p);
                vertexValue.mycolor = tmp[t];
            }else{

                int  flag = 0;
                for(int j = 0; j < vertexValue.neighbor_size; j++){ // judge is exists confiction
                    if(vertexValue.neighbors[j] == vertexValue.mycolor){
                        flag = 1;break;
                    }
                }

                if(flag == 1){//when this vertex's color is confict with neighbors',random pick a unused color
                   int p = 0;
                    for(int i = 0; i < color_num; i++){
                        for(int j = 0; j < vertexValue.neighbor_size; j++){
                            if(i != vertexValue.neighbors[j]){
                                tmp[p] = i;
                                p++;
                            }else continue;
                        }
                    }
                    int t = random(p);
                    vertexValue.mycolor = tmp[t];
                    * mutableValue() = vertexValue;
                    sendMessageToAllNeighbors(vertexValue.mycolor);
                    voteToHalt(); 
                }else{
                    voteToHalt();
                }   
            }
            // double acc = fabs(getValue() - val);
            // accumulateAggr(0, &acc);
        }
        
    }
};

class VERTEX_CLASS_NAME(Graph): public Graph {
public:
    VERTEX_CLASS_NAME(Aggregator)* aggregator;

public:
    // argv[0]: PageRankVertex.so
    // argv[1]: <input path>
    // argv[2]: <output path>
    void init(int argc, char* argv[]) {

        setNumHosts(5);
        setHost(0, "localhost", 1411);
        setHost(1, "localhost", 1421);
        setHost(2, "localhost", 1431);
        setHost(3, "localhost", 1441);
        setHost(4, "localhost", 1451);

        srand((unsigned)time(0));

        if (argc < 3) 
{           printf ("Usage: %s <input path> <output path>\n", argv[0]);
           exit(1);
        }

        m_pin_path = argv[1];
        m_pout_path = argv[2];
        char * v0id_s = argv[3];
        char * color_num_s = argv[4];

        color_num = atoi(color_num_s);
        aggregator = new VERTEX_CLASS_NAME(Aggregator)[1];
        regNumAggr(1);
        regAggr(0, &aggregator[0]);
    }

    void term() {
        delete[] aggregator;
    }
};

/* STOP: do not change the code below. */
extern "C" Graph* create_graph() {
    Graph* pgraph = new VERTEX_CLASS_NAME(Graph);

    pgraph->m_pin_formatter = new VERTEX_CLASS_NAME(InputFormatter);
    pgraph->m_pout_formatter = new VERTEX_CLASS_NAME(OutputFormatter);
    pgraph->m_pver_base = new VERTEX_CLASS_NAME();

    return pgraph;
}

extern "C" void destroy_graph(Graph* pobject) {
    delete ( VERTEX_CLASS_NAME()* )(pobject->m_pver_base);
    delete ( VERTEX_CLASS_NAME(OutputFormatter)* )(pobject->m_pout_formatter);
    delete ( VERTEX_CLASS_NAME(InputFormatter)* )(pobject->m_pin_formatter);
    delete ( VERTEX_CLASS_NAME(Graph)* )pobject;
}
