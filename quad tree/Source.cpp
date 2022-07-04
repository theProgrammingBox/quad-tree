#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

namespace olc
{
	struct rect
	{
		olc::vf2d pos;
		olc::vf2d size;

		rect(const olc::vf2d& p = { 0.0f, 0.0f }, const olc::vf2d& s = { 1.0f, 1.0f }) : pos(p), size(s)
		{

		}

		constexpr bool Contains(const olc::vf2d& point) const
		{
			return point.x >= pos.x &&
				point.x < pos.x + size.x &&
				point.y >= pos.y &&
				point.y < pos.y + size.y;
		}

		constexpr bool Contains(const olc::rect& rect) const
		{
			return rect.pos.x >= pos.x &&
				rect.pos.x + rect.size.x < pos.x + size.x &&
				rect.pos.y >= pos.y &&
				rect.pos.y + rect.size.y < pos.y + size.y;
		}

		constexpr bool Overlaps(const olc::rect& rect) const
		{
			return rect.pos.x < pos.x + size.x &&
				rect.pos.x + rect.size.x > pos.x &&
				rect.pos.y < pos.y + size.y &&
				rect.pos.y + rect.size.y > pos.y;
		}
	};
};

constexpr uint16_t MAX_DEPTH = 8;

template <typename OBJECT_TYPE>
struct QuadTreeItemLocation
{
	typename std::list<std::pair<olc::rect, OBJECT_TYPE>>* container;
	typename std::list<std::pair<olc::rect, OBJECT_TYPE>>::iterator iterator;
};

template <typename OBJECT_TYPE>
class QuadTree
{
public:
	QuadTree(const olc::rect& rect = { {0.0f, 0.0f}, {100.0f, 100.0f} }, const uint16_t nDepth = 0)
	{
		depth = nDepth;
		Resize(rect);
	}

	void Resize(const olc::rect& rect)
	{
		Clear();
		olc::vf2d vChildSize = rect.size / 2.0f;

		childRect =
		{
			olc::rect(rect.pos, vChildSize),
			olc::rect({rect.pos.x + vChildSize.x, rect.pos.y}, vChildSize),
			olc::rect({rect.pos.x, rect.pos.y + vChildSize.y}, vChildSize),
			olc::rect(rect.pos + vChildSize, vChildSize)
		};

	}

	void Clear()
	{
		items.clear();

		for (int i = 0; i < 4; i++)
		{
			if (childQuadTrees[i])
			{
				childQuadTrees[i]->Clear();
			}
			childQuadTrees[i].reset();
		}
	}

	QuadTreeItemLocation<OBJECT_TYPE> Insert(const OBJECT_TYPE& item, const olc::rect& itemRect)
	{
		for (int i = 0; i < 4; i++)
		{
			if (childRect[i].Contains(itemRect))
			{
				if (depth + 1 < MAX_DEPTH)
				{
					if (!childQuadTrees[i])
					{
						childQuadTrees[i] = std::make_shared <QuadTree<OBJECT_TYPE>>(childRect[i], depth + 1);
					}
					return childQuadTrees[i]->Insert(item, itemRect);
				}
			}
		}
		items.push_back({ itemRect, item });
		QuadTreeItemLocation<OBJECT_TYPE> location = { &items, std::prev(items.end()) };
		return location;
	}

	std::list<OBJECT_TYPE> Search(const olc::rect& rect) const
	{
		std::list<OBJECT_TYPE> list;
		Search(rect, list);
		return list;
	}

	void Search(const olc::rect& rect, std::list<OBJECT_TYPE>& list) const
	{
		for (const auto& p : items)
		{
			if (rect.Overlaps(p.first))
			{
				list.push_back(p.second);
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (childQuadTrees[i])
			{
				if (rect.Contains(childRect[i]))
				{
					childQuadTrees[i]->GetItems(list);
				}
				else if (childRect[i].Overlaps(rect))
				{
					childQuadTrees[i]->Search(rect, list);
				}
			}
		}
	}

	void GetItems(std::list<OBJECT_TYPE>& list) const
	{
		for (const auto& p : items)
		{
			list.push_back(p.second);
		}

		for (int i = 0; i < 4; i++)
		{
			if (childQuadTrees[i])
			{
				childQuadTrees[i]->GetItems(list);
			}
		}
	}

	void Remove(OBJECT_TYPE item)
	{
		for (auto it = items.begin(); it != items.end(); it++)
		{
			if (it->second == item)
			{
				items.erase(it);
				return;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (childQuadTrees[i])
			{
				childQuadTrees[i]->Remove(item);
			}
		}
	}

protected:
	uint16_t depth = 0;
	std::array<olc::rect, 4> childRect{};
	std::array<std::shared_ptr<QuadTree<OBJECT_TYPE>>, 4> childQuadTrees{};
	std::list<std::pair<olc::rect, OBJECT_TYPE>> items;
};

template <typename OBJECT_TYPE>
struct QuadTreeItem
{
	OBJECT_TYPE item;
	QuadTreeItemLocation<typename std::list<QuadTreeItem<OBJECT_TYPE>>::iterator> location;
};

template <typename OBJECT_TYPE>
class QuadTreeContainer
{
	using QuadTreeContainer_ = std::list<QuadTreeItem<OBJECT_TYPE>>;

public:
	QuadTreeContainer(const olc::rect& rect = { {0.0f, 0.0f}, { 100.0f, 100.0f } }, const uint16_t nDepth = 0) : rootQuadTree(rect, nDepth)
	{

	}

	void Resize(const olc::rect& rect)
	{
		rootQuadTree.Resize(rect);
	}

	uint16_t size() const
	{
		return items.size();
	}

	bool empty() const
	{
		return items.empty();
	}

	void clear()
	{
		rootQuadTree.clear();
		items.clear();
	}

	void Insert(const OBJECT_TYPE& item, const olc::rect& itemRect)
	{
		QuadTreeItem<OBJECT_TYPE> newItem;
		newItem.item = item;
		items.push_back(newItem);
		items.back().location = rootQuadTree.Insert(std::prev(items.end()), itemRect);
	}

	std::list<typename QuadTreeContainer_::iterator> Search(const olc::rect& rect) const
	{
		std::list<typename QuadTreeContainer_::iterator> listItemPointers;
		rootQuadTree.Search(rect, listItemPointers);
		return listItemPointers;
	}

	void Remove(typename QuadTreeContainer_::iterator& item)
	{
		item->location.container->erase(item->location.iterator);
		items.erase(item);
	}

	void Relocate(typename QuadTreeContainer_::iterator& item, const olc::rect& itemRect)
	{
		item->location.container->erase(item->location.iterator);
		item->location = rootQuadTree.Insert(item, itemRect);
	}

protected:
	QuadTreeContainer_ items;
	QuadTree<typename QuadTreeContainer_::iterator> rootQuadTree;
};

class Example_QuadTree : public olc::PixelGameEngine
{
public:
	Example_QuadTree()
	{
		sAppName = "QuadTree";
	}

	bool OnUserCreate() override
	{
		tv.Initialise({ ScreenWidth(), ScreenHeight() });
		treeObjects.Resize(olc::rect({ 0.0f, 0.0f }, { fArea, fArea }));

		auto rand_float = [](const float a, const float b)
		{
			return float(rand()) / float(RAND_MAX) * (b - a) + a;
		};

		for (int i = 0; i < 1000000; i++)
		{
			SomeObjectWithArea ob;
			ob.vPos = { rand_float(0.0f, fArea), rand_float(0.0f, fArea) };
			ob.vSize = { rand_float(0.1f, 100.0f), rand_float(0.1f, 100.0f) };
			ob.colour = olc::Pixel(rand() % 256, rand() % 256, rand() % 256);

			treeObjects.Insert(ob, olc::rect(ob.vPos, ob.vSize));
		}

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		tv.HandlePanAndZoom(0);

		if (GetMouseWheel() > 0 || GetKey(olc::Key::Q).bHeld)
		{
			fSearchSize += 10.0f;
		}
		else if (GetMouseWheel() < 0 || GetKey(olc::Key::A).bHeld)
		{
			fSearchSize -= 10.0f;
		}
		fSearchSize = std::min(std::max(fSearchSize, 10.0f), 500.0f);

		olc::rect rScreen = { tv.GetWorldTL(), tv.GetWorldBR() - tv.GetWorldTL() };
		uint16_t nObjectCount = 0;
		olc::vf2d vMouse = tv.ScreenToWorld(GetMousePos());
		olc::vf2d vSearchArea = { fSearchSize, fSearchSize };
		olc::rect m(vMouse - vSearchArea / 2.0f, vSearchArea);

		auto r = treeObjects.Search(m);

		if (GetKey(olc::Key::BACK).bHeld)
		{
			for (auto& p : r)
			{
				treeObjects.Remove(p);
			}
		}

		Clear(olc::BLACK);

		auto tpStart = std::chrono::system_clock::now();
		for (const auto& object : treeObjects.Search(rScreen))
		{
			tv.FillRect(object->item.vPos, object->item.vSize, object->item.colour);
			nObjectCount++;
		}
		std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;


		std::string sOutput = "Quadtree " + std::to_string(nObjectCount) + "/" + std::to_string(treeObjects.size()) + " in " + std::to_string(duration.count());
		DrawStringDecal({ 4, 4 }, sOutput, olc::BLACK, { 4.0f, 8.0f });
		DrawStringDecal({ 2, 2 }, sOutput, olc::WHITE, { 4.0f, 8.0f });

		tv.FillRectDecal(m.pos, m.size, olc::Pixel(255, 255, 255, 100));

		return true;
	}

protected:
	olc::TransformedView tv;

	struct SomeObjectWithArea
	{
		olc::vf2d vPos;
		olc::vf2d vVel;
		olc::vf2d vSize;
		olc::Pixel colour;
	};

	QuadTreeContainer<SomeObjectWithArea> treeObjects;
	float fArea = 100000.0f;
	float fSearchSize = 50.0f;
};


int main()
{
	Example_QuadTree demo;
	if (demo.Construct(1280, 960, 1, 1, false, false))
		demo.Start();
	return 0;
}