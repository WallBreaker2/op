#pragma once

#include<list>
#include<Eigen/Dense>
#include <set>
#include <algorithm>
#include <list>
#include <vector>

using std::list;
using namespace Eigen;
using std::set;
using std::list;
using std::vector;
class AStar {
public:
	struct Node {
		int F, G;
		Vector2i pos;
		Node* parent;
		//Node(int)
		Node() :F(0), G(0), parent{ nullptr }{

		}

		/*Node& operator=(const Node& rhs) {
			F = rhs.F;
			G = rhs.G;
			H = rhs.H;
			pos = rhs.pos;
			return *this;
		}*/

	};
	struct Nodeless
	{
		bool operator()(const Node& lhs, const Node& rhs) const {
			return lhs.pos[0] < rhs.pos[0] || (lhs.pos[0] == rhs.pos[0] && lhs.pos[1] < rhs.pos[1]);
		}
	};
	struct Vec2less {
		bool operator()(const Vector2i& lhs, const Vector2i& rhs) const {
			return lhs[0] < rhs[0] || (lhs[0] == rhs[0] && lhs[1] < rhs[1]);
		}
	};
	AStar() {
		_start[0] = _start[1] = 0;
		_end[0] = _end[1] = 0;
		//_start < _end;

	}
	bool outside(Vector2i pos) {
		return pos[0]<0 || pos[0]>_mapSize[0] || pos[1]<0 || pos[1]>_mapSize[1];
	}
	void set_map(int w,int h, const vector<Vector2i>& wall) {
		_mapSize[0] = w; _mapSize[1] = h;
		_wallset.clear();
		for (auto&it : wall) {
			_wallset.insert(it);
		}
		/*	Vector2i tp;
			for (int i = 0; i <= mapSize[0];++i) {
				_wallset.insert({ i,0 });
				_wallset.insert({ 0,i });
				_wallset.insert({ mapSize[0],i });
				_wallset.insert({ i,mapSize[1] });
			}*/
	}
	void findpath(int beginX, int beginY,int endX,int endY, list<Vector2i>&path) {
		_openset.clear();
		_closedset.clear();
		path.clear();
		_start[0] = beginX; _start[1] = beginY;
		_end[0] = endX; _end[1] = endY;
		if (outside(_start) || outside(_end))
			return;
	
		Node curr_node;
		curr_node.pos = _start;
		_openset.insert(curr_node);
		while (!_openset.empty()) {
			auto S = std::min_element(_openset.begin(), _openset.end(), [](const Node& lhs, const Node& rhs) {return lhs.F < rhs.F; });
			curr_node = *S;
			_closedset.insert(curr_node);
			_openset.erase(S);
			if (curr_node.pos == _end) {//get it!
				break;
			}
			//生成所有相邻节点
			Node temp;
			for (int i = 0; i < 8; ++i) {
				temp.pos = curr_node.pos + _dir4[i];
				temp.G = curr_node.G + 1;
				int H = std::abs(temp.pos[0] - _end[0]) + std::abs(temp.pos[1] - _end[1]);
				temp.F = temp.G + H;
				if (_closedset.count(temp))
					continue;
				if (_wallset.count(temp.pos))
					continue;
				if (outside(temp.pos))
					continue;
				auto it = _openset.find(temp);
				//如果节点不在开放节点
				if (it == _openset.end()) {

					temp.parent = (Node*)&(*_closedset.find(curr_node));
					_openset.insert(temp);
				}
				else {
					//更新开放节点
					if (it->F > temp.F) {
						auto ptr = (Node*)&(*it);
						ptr->F = temp.F;
					}
				}
			}
		}
		//获取路径
		curr_node.pos = _end;
		auto endit = _closedset.find(curr_node);
		if (endit != _closedset.end()) {
			auto pnode = (Node*)&(*endit);
			while (pnode)
			{
				_pathset.insert(pnode->pos);
				path.push_back(pnode->pos);
				pnode = pnode->parent;
			}
		}



	}
	
private:
	//Eigen::
	Vector2i _start, _end;
	//地图大小
	Vector2i _mapSize;
	//开放节点
	set<Node, Nodeless> _openset;
	set<Node, Nodeless> _closedset;
	//墙节点
	set<Vector2i, Vec2less> _wallset;
	//路径节点
	set<Vector2i, Vec2less> _pathset;
	//方向
	Vector2i const  _dir4[8] = { {0,1},{0,-1},{-1,0},{1,0},{1,1},{-1,1},{-1,-1},{1,-1} };
private:
};

