#pragma once

#include<list>
#include <set>
#include <algorithm>
#include <list>
#include <vector>

using std::list;

using std::set;
using std::list;
using std::vector;
class AStar {
public:
	struct Vec2i { 
		int x, y;
		bool operator==(const Vec2i& rhs)const {
			return x == rhs.x&&y == rhs.y;
		}
		Vec2i operator+(const Vec2i& rhs)const {
			return Vec2i{ x + rhs.x,y + rhs.y };
		}
	};
	struct Node {
		int F, G;
		Vec2i pos;
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
			return lhs.pos.x < rhs.pos.y || (lhs.pos.x == rhs.pos.x && lhs.pos.y < rhs.pos.y);
		}
	};
	struct Vec2less {
		bool operator()(const Vec2i& lhs, const Vec2i& rhs) const {
			return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
		}
	};
	AStar() {
		_start.x = _start.y = 0;
		_end.x = _end.y = 0;
		//_start < _end;

	}
	bool outside(Vec2i pos) {
		return pos.x<0 || pos.x>_mapSize.x || pos.y<0 || pos.y>_mapSize.y;
	}
	void set_map(int w,int h, const vector<Vec2i>& wall) {
		_mapSize.x = w; _mapSize.y = h;
		_wallset.clear();
		for (auto&it : wall) {
			_wallset.insert(it);
		}
		/*	Vector2i tp;
			for (int i = 0; i <= mapSize.x;++i) {
				_wallset.insert({ i,0 });
				_wallset.insert({ 0,i });
				_wallset.insert({ mapSize.x,i });
				_wallset.insert({ i,mapSize.y });
			}*/
	}
	void findpath(int beginX, int beginY,int endX,int endY, list<Vec2i>&path) {
		_openset.clear();
		_closedset.clear();
		path.clear();
		_start.x = beginX; _start.y = beginY;
		_end.x = endX; _end.y = endY;
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
				int H = std::abs(temp.pos.x - _end.x) + std::abs(temp.pos.y - _end.y);
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
	Vec2i _start, _end;
	//地图大小
	Vec2i _mapSize;
	//开放节点
	set<Node, Nodeless> _openset;
	set<Node, Nodeless> _closedset;
	//墙节点
	set<Vec2i, Vec2less> _wallset;
	//路径节点
	set<Vec2i, Vec2less> _pathset;
	//方向
	Vec2i const  _dir4[8] = { {0,1},{0,-1},{-1,0},{1,0},{1,1},{-1,1},{-1,-1},{1,-1} };
private:
};

