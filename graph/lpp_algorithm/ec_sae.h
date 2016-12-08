#ifndef LPPA_EC_SAE_H
#define LPPA_EC_SAE_H

/*
SA�㷨��˽ģ��Ϊ k,�Ȱ�ȫ, ����̩ɭ����ν�����Ȥ�㻮��
�������̱�¶����Asuit,�����㷨�ڿͻ���ִ��
���û�������������Ĺ�����,�������ѡ�������λ��-����ȱʧֵ
��ѡ���·����㣬���ѡ�����ж���͵���������

�㷨����Ϊ��ÿ�μ���һ���ٽ���Ȥ�㣬ֱ��������˽ģ��
���ܱ�֤l������
Ϊ����Ӧ�µ���˽Ŀ�ꡢ����Ƚϲ�ѯЧ�ʣ�
���õ�����������������Ӱ���㷨����
ÿ��������չʱ������һ���ٽ��߶�����һ��λ��
s_type_poi_pop/poi_pop_all >s_require �Ķ���Ϊ��������
*/

#include "../edge_cluster_graph.h"
#include "util.h"
#include <ctime>
#include <cstdlib>
#include <vector>
#include <set>
#include <map>
using namespace std;

class Lppa_ecsa_e
{
public:
	Lppa_ecsa_e(EC_Graph *p_graph)
	{
		this->p_graph = p_graph;
		anonymization_time_total = 0;
		cnt_of_failure = 0;
		cnt_of_success = 0;
		l_max = 30;
		//��ӡ��ǰ��ͼ״̬
	}
	//�����㷨
	void lpp()
	{
		const vector<EC_Node*> &nodes = p_graph->get_nodes();
		LBS_User *pu = nullptr;
		clock_t start, finish;
		is_node_selecteds = new bool[nodes.size()];
		is_node_candidate = new bool[nodes.size()];
		start = clock();
		for (int i = 0; i < nodes.size(); i++) {
			for (int j = 0; j < nodes[i]->get_users().size(); j++) {
				pu = nodes[i]->get_users()[j];
				users.push_back(nodes[i]->get_users()[j]);
				sa(pu, nodes[i]);
			}

		}
		finish = clock();
		delete []is_node_selecteds;
		delete []is_node_candidate;
		double time_cost = (double)(finish - start) / CLOCKS_PER_SEC;
		cout << "total time consume:" << time_cost << endl;
		cout << "success:" << cnt_of_success << endl;
		cout << "failure:" << cnt_of_failure << endl;
		//ͳ��
		//��һ�齫����������ʱ�䣬ֻ�����ڷ����㷨����
		//���Žڵ㴦������ʱδ��״̬,ֻ��Ϊ����ָ��
		cout << "calculate open vetex begin:" << endl;
		set<Node*> node_set;
		set<Node*>::iterator it_node_set;
		vector<Edge*> adj_edges,cloak_set;
		vector<Edge*>::iterator it_cloak_set;
		Node *pn1, *pn2;
		int all_openvetex_cnt = 0, all_edge_size = 0;

		int user_cnt = vv_cloak_sets.size();
		for (int i = 0; i < vv_cloak_sets.size(); i++) {
			cloak_set.clear();
			for (int j=0; j<vv_cloak_sets[i].size();j++) {
				cloak_set.push_back(vv_cloak_sets[i][j]->get_src_edge());
			}
			node_set.clear();
			//init inner_node set
			for (int j = 0; j < cloak_set.size(); j++) {
				Node *pn1, *pn2;
				pn1 = cloak_set[j]->getNode1();
				pn2 = cloak_set[j]->getNode2();
				if (node_set.find(pn1) == node_set.end()) {
					node_set.insert(pn1);
				}
				if (node_set.find(pn2) == node_set.end()) {
					node_set.insert(pn2);
				}
			}
			all_edge_size += cloak_set.size();
			int cnt_of_circle = 0, cnt_of_openvetex = 0;

			for (it_node_set = node_set.begin(); it_node_set != node_set.end(); it_node_set++) {
				adj_edges = (*it_node_set)->getAdjEdges();
				for (int j = 0; j < adj_edges.size(); j++) { //�ж������ٽ����Ƿ���������
					if (!vector_find(cloak_set, adj_edges[j])) { //û�ҵ�,�������ж����ڣ�
						cnt_of_openvetex++;
						break;
					}
				}
			}

			open_vetex_cnt.push_back(cnt_of_openvetex);
			all_openvetex_cnt += cnt_of_openvetex;
			//cout << "open vetex: "<< cnt_of_openvetex << endl;
			if (cnt_of_openvetex > 60) cout << "###" << endl;
		}
		cout << "average openvetex: " << (double)all_openvetex_cnt / user_cnt << endl;
		cout << "average cloak size: " << (double)all_edge_size / user_cnt << endl;

	}
	//Ϊÿ���û�
	void sa(LBS_User *&pu, EC_Node *pn)
	{
		k = 0;
		l = 0;
		accumulate_pop = 0.0;
		accumulate_svalue = 0.0;
		memset(is_node_selecteds, false, p_graph->get_nodes().size());
		memset(is_node_candidate, false, p_graph->get_nodes().size());
		double w1 = 2, w2 = 1; // w2���ڽṹ����Ҫ�̶�
		bool is_satisfied = false;
		double accumulate_svalue = 0, accumulate_pop = 0;
		vector<EC_Node*> cloak_set;
		map<EC_Node*, pair<double, double>> candidate_map;

		const vector<double> &sensitive_vals = pu->get_sensitive_vals();
		is_satisfied = add_node_to_cloakset(pu, pn, cloak_set, candidate_map);
		for (int i = 1; i < l_max && !is_satisfied; i++) {
			map<EC_Node*, pair<double, double>>::iterator it_candidate, it_maximal_score;
			double k_score = 0, s_score = 0, struct_score = 0;
			if (candidate_map.size() < 1) break;
			if (accumulate_pop > 0.001 && accumulate_svalue / accumulate_pop > pu->get_s()) { //���������k����Ҫ�̶�
				w1 = 2;  //����δ����
			}
			else {
				w1 = 1;
			}
			double minimal_miss_s = 0x7fffffff, maximal_miss_s = 0, maximal_score = 0;
			//--
			for (it_candidate = candidate_map.begin(); it_candidate != candidate_map.end(); it_candidate++) { //�Ѿ������˵ľͲ��ü���
				double candidate_svalue = it_candidate->second.first;
				double candidate_pop = it_candidate->second.second;
				double missed_value = candidate_svalue / pu->get_s() - candidate_pop;
				if (missed_value < minimal_miss_s) minimal_miss_s = missed_value;
				if (missed_value > maximal_miss_s) maximal_miss_s = missed_value;
			}
			for (it_candidate = candidate_map.begin(); it_candidate != candidate_map.end(); it_candidate++) { //�Ѿ������˵ľͲ��ü���

				if (k >= pu->get_k()) {
					k_score = 1;
				}
				else {
					k_score = ((double)k + (it_candidate->first->get_users().size())) / pu->get_k();
					k_score = k_score < 1 ? k_score : 1;
				}
				if (fabs(maximal_miss_s - minimal_miss_s) < 0.001) {
					s_score = 1;
				}
				else {
					double candidate_svalue = it_candidate->second.first;
					double candidate_pop = it_candidate->second.second;
					double missed_value = candidate_svalue / pu->get_s() - candidate_pop;
					s_score = w1 * (maximal_miss_s - missed_value) / (maximal_miss_s - minimal_miss_s);
				}
				//struct
				vector<EC_Node*> &this_adj_nodes = it_candidate->first->get_adj_nodes();
				int cnt_of_inner_node = 0;

				//����ƿ��
				for (int i = 0; i < this_adj_nodes.size(); i++) {
					if (is_node_selecteds[this_adj_nodes[i]->get_id()]) {
						cnt_of_inner_node++;
					}
					if (cnt_of_inner_node > 1) break;
				}

				////openvetex������ָ��
				if (cnt_of_inner_node > 1) { //��ͼ�ɻ�
					struct_score = 0.1;

				}
				else {
					struct_score = 0;
				}
				//struct_score = 0;
				//struct_score = (double)rand() / RAND_MAX *0.3;
				double score = k_score + s_score + struct_score;
				//ѡ��÷���ߵ�
				if (score > maximal_score) {
					maximal_score = score;
					it_maximal_score = it_candidate;
				}
			}

			is_satisfied = add_node_to_cloakset(pu, it_maximal_score->first, cloak_set, candidate_map);
			candidate_map.erase(it_maximal_score);
			is_node_candidate[it_maximal_score->first->get_id()] = false;

		}//end for
		if (is_satisfied) { //�ɹ�
			is_success.push_back(true);
			cnt_of_success++;
		}
		else {
			is_success.push_back(false);
			cnt_of_failure++;
		}
		vv_cloak_sets.push_back(cloak_set);
	}


	bool add_node_to_cloakset(LBS_User *&pu, EC_Node *new_node, vector<EC_Node*> &cloak_set, map<EC_Node*, pair<double, double>> &candidate_map)
	{
		//�����������ͺ�ѡ��
		const vector<double> &sensitive_vals = pu->get_sensitive_vals();
		cloak_set.push_back(new_node);
		is_node_selecteds[new_node->get_id()] = true;
		Node *pn1, *pn2;
		vector<EC_Node*> &adj_nodes = new_node->get_adj_nodes(); //ֻӰ��ռ���Ĳ���

		for (int i = 0; i < adj_nodes.size(); i++) {
			if (!is_node_candidate[adj_nodes[i]->get_id()] && !is_node_selecteds[adj_nodes[i]->get_id()]) { //�����ѡ��
				double candidate_edge_svalue = 0.0, candidate_edge_pop = 0.0;
				const vector<Poi*> &e_pois = adj_nodes[i]->get_pois();
				for (int j = 0; j < e_pois.size(); j++) { //�����Ȥ��
					int poi_type = (int)e_pois[j]->get_type();
					candidate_edge_svalue += (sensitive_vals[poi_type] * e_pois[j]->get_pop());
					candidate_edge_pop += e_pois[j]->get_pop();
				}
				candidate_map[adj_nodes[i]] = make_pair(candidate_edge_svalue, candidate_edge_pop);
				is_node_candidate[adj_nodes[i]->get_id()] = true;
			}
		}
		//���㵱ǰ״̬
		k += new_node->get_users().size();
		l++;
		double candidate_svalue = 0.0, candidate_pop = 0.0;
		if (!is_node_candidate[new_node->get_id()]) { //��һ��ִ��
			const vector<Poi*> &e_pois = new_node->get_pois();
			for (int j = 0; j < e_pois.size(); j++) { //�����Ȥ��
				int poi_type = (int)e_pois[j]->get_type();
				candidate_svalue += (sensitive_vals[poi_type] * e_pois[j]->get_pop());
				candidate_pop += e_pois[j]->get_pop();
			}
		}
		else {
			candidate_svalue = candidate_map[new_node].first;
			candidate_pop = candidate_map[new_node].second;
		}
		accumulate_svalue += candidate_svalue;
		accumulate_pop += candidate_pop;
		if (k < pu->get_k() || l < pu->get_l() || (accumulate_pop > 0.001 && accumulate_svalue / accumulate_pop > pu->get_s())) return false;
		return true;
	}
private:
	//ÿ�������û�
	int k, l;
	bool *is_node_selecteds; //hash set�����ٶȺ���
	bool *is_node_candidate; 
	double accumulate_svalue, accumulate_pop;
	//ͳ����Ϣ
	vector<bool> is_success;
	vector<LBS_User*> users;
	vector<vector<EC_Node*>> vv_cloak_sets;
	double anonymization_time_total;
	int cnt_of_success;
	int cnt_of_failure;
	vector<int> open_vetex_cnt; //��������Ȩֵ��ȥ����
	int l_max; //���������ѡ��l_max����,����ʧ��
	EC_Graph *p_graph;
};


#endif