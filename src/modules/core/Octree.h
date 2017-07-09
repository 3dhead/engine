/**
 * @file
 */

#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include "AABB.h"
#include "Frustum.h"
#include "Trace.h"
#include "GLM.h"

namespace core {

extern core::AABB<int> computeAABB(const Frustum& area, const glm::vec3& minSize);

/**
 * @note Given NODE type must implement @c aabb() and return core::AABB<TYPE>
 */
template<class NODE, typename TYPE = int>
class Octree {
public:
	typedef typename std::vector<NODE> Contents;

	class OctreeNode;
	struct IOctreeListener {
		virtual ~IOctreeListener() {}
		virtual void onNodeCreated(const OctreeNode& parent, const OctreeNode& child) const {}
	};

	class OctreeNode {
		friend class Octree;
	private:
		int _maxDepth;
		int _depth;
		const Octree* _octree;
		const AABB<TYPE> _aabb;
		Contents _contents;
		std::vector<OctreeNode> _nodes;

		template<class FUNC>
		inline void visit(FUNC&& func) const {
			func(*this);
			for (const OctreeNode& node : _nodes) {
				node.visit(func);
			}
		}

		void createNodes() {
			if (_depth >= _maxDepth) {
				return;
			}
			if (glm::all(glm::lessThanEqual(aabb().getWidth(), glm::one<glm::tvec3<TYPE> >()))) {
				return;
			}

			std::array<AABB<TYPE>, 8> subareas;
			aabb().split(subareas);
			_nodes.reserve(subareas.size());
			for (size_t i = 0u; i < subareas.size(); ++i) {
				_nodes.emplace_back(OctreeNode(subareas[i], _maxDepth, _depth + 1, _octree));
				if (_octree->_listener != nullptr) {
					_octree->_listener->onNodeCreated(*this, _nodes.back());
				}
			}
		}
	public:
		static inline AABB<TYPE> aabb(const typename std::remove_pointer<NODE>::type* item) {
			return item->aabb();
		}

		static inline AABB<TYPE> aabb(const typename std::remove_pointer<NODE>::type& item) {
			return item.aabb();
		}

		OctreeNode(const AABB<TYPE>& bounds, int maxDepth, int depth, const Octree* octree) :
				_maxDepth(maxDepth), _depth(depth), _octree(octree), _aabb(bounds) {
		}

		inline int depth() const {
			return _depth;
		}

		inline int count() const {
			int count = 0;
			for (const typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				count += node.count();
			}
			count += _contents.size();
			return count;
		}

		inline const AABB<TYPE>& aabb() const {
			return _aabb;
		}

		inline const Contents& getContents() const {
			return _contents;
		}

		void getAllContents(Contents& results) const {
			for (const typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				node.getAllContents(results);
			}

			results.insert(results.end(), _contents.begin(), _contents.end());
		}

		bool remove(const NODE& item) {
			const AABB<TYPE>& area = aabb(item);
			if (!aabb().containsAABB(area)) {
				return false;
			}
			for (typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				if (!node.remove(item)) {
					continue;
				}
				return true;
			}

			typename Contents::iterator i = std::find(_contents.begin(), _contents.end(), item);
			if (i == _contents.end()) {
				return false;
			}
			_contents.erase(i);
			return true;
		}

		bool insert(const NODE& item) {
			const AABB<TYPE>& area = aabb(item);
			if (!aabb().containsAABB(area)) {
				return false;
			}

			if (_nodes.empty()) {
				createNodes();
			}

			for (typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				if (!node.insert(item)) {
					continue;
				}
				return true;
			}

			_contents.push_back(item);
			return true;
		}

		inline bool isEmpty() const {
			return _nodes.empty() && _contents.empty();
		}

		void query(const AABB<TYPE>& queryArea, Contents& results) const {
			for (const NODE& item : _contents) {
				if (intersects(queryArea, aabb(item))) {
					results.push_back(item);
				}
			}

			for (const typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				if (node.isEmpty()) {
					continue;
				}

				const AABB<TYPE>& aabb = node.aabb();
				if (aabb.containsAABB(queryArea)) {
					node.query(queryArea, results);
					// the queried area is completely part of the node - so no other node can be involved
					break;
				}

				if (queryArea.containsAABB(aabb)) {
					node.getAllContents(results);
					// the whole node content is part of the query
					continue;
				}

				if (intersects(aabb, queryArea)) {
					node.query(queryArea, results);
				}
			}
		}

		void query(const Frustum& queryArea, const AABB<TYPE>& queryAreaAABB, Contents& results) const {
			for (const NODE& item : _contents) {
				const auto& itemAABB = aabb(item);
				if (queryArea.isVisible(itemAABB.mins(), itemAABB.maxs())) {
					results.push_back(item);
				}
			}

			for (const typename Octree<NODE, TYPE>::OctreeNode& node : _nodes) {
				if (node.isEmpty()) {
					continue;
				}

				const AABB<TYPE>& aabb = node.aabb();
				if (aabb.containsAABB(queryAreaAABB)) {
					node.query(queryArea, queryAreaAABB, results);
					// the queried area is completely part of the node - so no other node can be involved
					break;
				}

				const FrustumResult result = queryArea.test(aabb.mins(), aabb.maxs());
				if (FrustumResult::Intersect == result) {
					// some children might be visible - but other nodes might also still contribute
					node.query(queryArea, queryAreaAABB, results);
				} else if (FrustumResult::Inside == result) {
					// the whole node content is part of the query
					node.getAllContents(results);
				}
			}
		}
	};
private:
	OctreeNode _root;
	// dirty flag can be used for query caches
	bool _dirty = false;
	const IOctreeListener* _listener = nullptr;

	template<class VISITOR>
	void visit(const Frustum& queryArea, const AABB<TYPE>& queryAABB, VISITOR&& visitor, const glm::vec<3, TYPE>& minSize) const {
		const glm::tvec3<TYPE>& mins = queryAABB.mins();
		const glm::tvec3<TYPE>& width = queryAABB.getWidth();
		const TYPE maxX = mins.x + width.x;
		const TYPE maxY = mins.y + width.y;
		const TYPE maxZ = mins.z + width.z;
		glm::tvec3<TYPE> qmins(glm::uninitialize);
		for (qmins.x = mins.x; qmins.x < maxX; qmins.x += minSize.x) {
			for (qmins.y = mins.y; qmins.y < maxY; qmins.y += minSize.y) {
				for (qmins.z = mins.z; qmins.z < maxZ; qmins.z += minSize.z) {
					const glm::tvec3<TYPE> qmaxs(qmins + minSize);
					if (!queryArea.isVisible(qmins, qmaxs)) {
						continue;
					}
					if (!visitor(qmins, qmaxs)) {
						break;
					}
				}
			}
		}
	}

public:
	Octree(const AABB<TYPE>& aabb, int maxDepth = 10) :
			_root(aabb, maxDepth, 0, this) {
	}

	inline int count() const {
		return _root.count();
	}

	inline bool insert(const NODE& item) {
		if (_root.insert(item)) {
			_dirty = true;
			return true;
		}
		return false;
	}

	inline bool remove(const NODE& item) {
		if (_root.remove(item)) {
			_dirty = true;
			return true;
		}
		return false;
	}

	inline const AABB<TYPE>& aabb() const {
		return _root.aabb();
	}

	inline void query(const AABB<TYPE>& area, Contents& results) const {
		core_trace_scoped(OctreeQuery);
		_root.query(area, results);
	}

	inline void query(const Frustum& area, Contents& results) const {
		core_trace_scoped(OctreeQuery);
		const AABB<float>& areaAABB = area.aabb();
		_root.query(area, AABB<TYPE>(areaAABB.mins(), areaAABB.maxs()), results);
	}

	/**
	 * @brief Executes the given visitor for all visible nodes in this octree.
	 * @note As there might not be nodes yet for the potential visible nodes, the visitor
	 * is just called with the center of the potential node
	 * @param area The frustum area to visit visible AABBs for
	 * @param visitor The visitor to execute if an AABB is visible in the given frustum
	 * @param minSize The mimimum size of the AABBs when the recursion is stopped.
	 */
	template<class VISITOR>
	inline void visit(const Frustum& area, VISITOR&& visitor, const glm::vec<3, TYPE>& minSize) {
		const glm::vec3 fminsize(minSize);
		const core::AABB<int>& aabb = computeAABB(area, fminsize);
		visit(area, aabb, visitor, minSize);
	}

	void setListener(const IOctreeListener* func) {
		_listener = func;
	}

	void clear() {
		const size_t size = _root._contents.size();
		_dirty = true;
		_root._contents.clear();
		_root._contents.reserve(size);
		_root._nodes.clear();
		_root._nodes.reserve(4);
	}

	inline void markAsClean() {
		_dirty = false;
	}

	inline bool isDirty() {
		return _dirty;
	}

	inline void getContents(Contents& results) const {
		results.clear();
		results.reserve(count());
		_root.getAllContents(results);
	}

	template<class FUNC>
	void visit(FUNC&& func) {
		_root.visit(func);
	}
};

#define CACHE 1
template<class NODE, typename TYPE>
class OctreeCache {
private:
	Octree<NODE, TYPE>& _tree;
#if CACHE
	std::unordered_map<AABB<TYPE>, typename Octree<NODE, TYPE>::Contents> _cache;
#endif
public:
	OctreeCache(Octree<NODE, TYPE>& tree) :
			_tree(tree) {
	}

	inline void clear() {
#if CACHE
		_cache.clear();
#endif
	}

	inline bool query(const AABB<TYPE>& area, typename Octree<NODE, TYPE>::Contents& contents) {
#if CACHE
		if (_tree.isDirty()) {
			_tree.markAsClean();
			clear();
		}
		// TODO: normalize to octree cells to improve the cache hits
		auto iter = _cache.find(area);
		if (iter != _cache.end()) {
			contents = iter->second;
			return true;
		}
		_tree.query(area, contents);
		_cache.insert(std::make_pair(area, contents));
#else
		_tree.query(area, contents);
#endif
		return false;
	}
};

#undef CACHE

}
