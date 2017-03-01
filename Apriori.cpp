#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <iterator>
#include <map>
#include <bitset>
#include <fstream>
#include <sstream>

template <class ItemType>
class Database
{
public:
	typedef std::set<ItemType> Itemset;

	virtual bool open() = 0;
	virtual bool close() = 0;
	virtual bool isDone() = 0;
	virtual Itemset nextTransaction() = 0;

	virtual ~Database() {}
};

template <class ItemType>
class FP
{
public:
	struct Node {
		Node()
		: parent(nullptr), neighbor(nullptr), supportCount(0), item(nullptr)
		{}

		Node* parent;
		Node* neighbor;
		std::vector<Node*> children;
		size_t supportCount;
		ItemType* item;

		Node* findChild(ItemType item)
		{
			for(Node* child : children) {
				if(*child->item == item) {
					return child;
				}
			}

			return nullptr;
		}
	};

	typedef std::set<ItemType> Itemset;
	typedef std::map<Itemset, size_t> Itemsets;

	FP(Database<ItemType>& database, size_t supportCount)
		: database(database), numberOfTransactions(0), supportCount(supportCount)
	{
	}

	void calculateFrequentItemsets()
	{
		createHeaderTable();
		createTree();
		mineTree();
		printTree();
	}

private:
	void printTree()
	{
		std::cout << "Header table: " << std::endl;
		for(auto& headerPair : headerTable) {
			std::cout << headerPair.first << " " << headerPair.second.first << std::endl;
		}
		std::cout << "_______________" << std::endl;

		std::cout << "Tree: " << std::endl;
		for(auto& headerPair : headerTable) {
			Node* neighbor = headerPair.second.second;
			while(neighbor != nullptr) {
				std::cout << *neighbor->item << ":" << neighbor->supportCount;

				if(neighbor->neighbor != nullptr) {
					std::cout << " -> ";
				}

				neighbor = neighbor->neighbor;
			}

			std::cout << std::endl;
		}
	}

	void mineTree()
	{

	}

	void createHeaderTable()
	{
		std::cout << "Initializing header table." << std::endl;

		database.open();

		while(!database.isDone()) {
			Itemset itemset = database.nextTransaction();

			for(auto& item : itemset)
			{
				if(headerTable.count(item) == 0) {
					headerTable[item] = std::make_pair(0, nullptr);
					order.push_back(item);
				}

				headerTable[item].first += 1;
				numberOfTransactions++;
			}
		}

		database.close();

		// TODO REMOVE INFREQUENT ITEMS ALREADY

		std::cout << "Found " << numberOfTransactions << " transactions, " << order.size() << " unique items." << std::endl;

		std::cout << "Sorting items: ";
		std::sort(order.begin(), order.end(), [this](const ItemType& a, const ItemType& b) -> bool { return this->itemSorter(a, b, true); });
		std::cout << "[DONE]" << std::endl;
	}

	void createTree()
	{
		std::cout << "Creating FP tree: ";

		database.open();

		while(!database.isDone()) {
			Itemset itemset = database.nextTransaction();
			std::vector<ItemType> items(itemset.begin(), itemset.end());

			std::sort(items.begin(), items.end(), [this](const ItemType& a, const ItemType& b) -> bool { return this->itemSorter(a, b); });

			// Every transaction starts insertion at root.
			Node* currentRoot = &root;

			for(auto& item : items) {
				Node* node = currentRoot->findChild(item);
				if(node == nullptr) {
					node = new Node;
					node->parent = currentRoot;
					node->item = new ItemType(item);

					// Else insert it as child.
					currentRoot->children.push_back(node);

					// Insert as first link in header table if none exists yet.
					if(headerTable[item].second == nullptr) {
						headerTable[item].second = node;
					} else {
						// Otherwise we follow the links and add as neighbor.
						Node* prev = headerTable[item].second;
						while(prev->neighbor != nullptr) {
							prev = prev->neighbor;
						}

						prev->neighbor = node;
					}
				}

				node->supportCount++;
				currentRoot = node;

			}
		}

		database.close();

		std::cout << "[DONE]" << std::endl;
	}

	bool itemSorter(const ItemType& a, const ItemType& b, bool invert = false)
	{
		if(invert) {
			return headerTable[a].first < headerTable[b].first;
		}

		return headerTable[a].first > headerTable[b].first;
	}

	std::map<ItemType, std::pair<size_t, Node*>> headerTable;

	size_t supportCount;
	size_t numberOfTransactions;
	Database<ItemType>& database;
	Node root;
	std::vector<ItemType> order;
};

template <class ItemType>
class Apriori
{
public:
	typedef std::set<ItemType> Itemset;

	typedef std::map<Itemset, size_t> Itemsets;

	Apriori(Database<ItemType>& database, size_t supportCount)
		: database(database), numberOfTransactions(0), supportCount(supportCount)
	{
	}

	void calculateFrequentItemsets()
	{
		Itemsets itemsets = getFrequent1Itemsets();
		Ls[1] = itemsets;

		std::cout << "Frequent 1 itemsets: " << Ls[1].size() << std::endl;


		Itemsets C;
		size_t i = 2;
		while(true) {
			C = generateCandidates(i);
			std::cout << "Generated " << i << "-itemsets candidates: " << C.size() << std::endl;
			if(C.size() == 0) {
				break;
			}
			filterCandidates(C, i);

			std::cout << "Frequent " << i << "-itemsets: " << C.size() << std::endl;

			if(C.size() != 0) {
				Ls[i] = C;
			} else {
				break;
			}
			i++;
		}


	}

	std::map<unsigned int, Itemsets>& getFrequentItemsets()
	{
		return Ls;
	}

private:
	std::vector<Itemset> getSubsets(unsigned int k, Itemset& itemset)
	{
		//std::cout << "finding " << k << " size subsets of set: " << itemset.size() << std::endl;
		//for(auto& item : itemset) {
		//	std::cout << item << ", ";
		//}
		//std::cout << std::endl;
		std::vector<Itemset> subsets;
		std::vector<Itemset> working;

		if(k == 0 || k > itemset.size()) {
			return subsets;
		}

		if(k == itemset.size()) {
			return std::vector<Itemset>{itemset};
		}


		for(auto& item : itemset) {
			subsets.push_back(Itemset{item});
		}

		for(int i = 1; i < k; i++) {
			for(auto iter1 = subsets.begin(); iter1 != subsets.end(); ++iter1) {
				auto start = iter1;
				start++;


				for(auto iter2 = start; iter2 != subsets.end(); ++iter2) {
					auto subset = *iter1;


					// k-1 are not equal
					auto iter4 = (*iter2).begin();
					bool notEqual = false;
					for(auto iter3 = subset.begin(); iter3 != --subset.end(); iter3++) {
						if(*iter3 != * iter4) {
							notEqual = true;
							break;
						}
						iter4++;
					}
					if(notEqual) {
						continue;
					}


					subset.insert(*(--iter2->end()));
					working.push_back(subset);
				}
			}

			subsets = working;
			working.clear();


		}

		return subsets;
	}

	void filterCandidates(Itemsets& C, unsigned int k)
	{
		database.open();

		while(!database.isDone()) {
			Itemset itemset = database.nextTransaction();


			//std::cout << "Starting calculation of subsets of size " << k << " from set of size " << itemset.size() << std::endl;
			auto subsets = getSubsets(k, itemset);
			//std::cout << "Number of subsets of size " << k << " in set of size " << itemset.size() << " : " << subsets.size() <<std::endl;


			for(auto& subset : subsets) {
				if(C.count(subset) == 0) {
					continue;
				}

				C[subset] += 1;
			}

			/*for(auto& candidate : C) {
				if(candidate.first.size() > itemset.size()) {
					continue;
				}

				bool found = false;
				auto item1 = candidate.first.begin();
				auto item2 = itemset.begin();
				while(!found && item1 != candidate.first.end() && item2 != itemset.end()) {
					if(*item1 != *item2) {
						item2++;
						continue;
					}

					if(item1 == --candidate.first.end()) {
						found = true;
					}
					item1++;
					item2++;
				}

				if(!found) {
					continue;
				}

				C[candidate.first] += 1;
			}*/
		}


		for(auto iter = C.begin(); iter != C.end(); ++iter) {
			if(iter->second < supportCount) {
				C.erase(iter);
			}
		}



		database.close();

	}

	Itemsets generateCandidates(unsigned int Ln)
	{
		auto L = Ls[Ln-1];

		Itemsets candidates;

		for(auto iter1 = L.begin(); iter1 != L.end(); ++iter1) {
			auto start = iter1;
			start++;
			for(auto iter2 = start; iter2 != L.end(); ++iter2) {
				Itemset candidate = iter1->first;

				// k-1 are not equal
				auto iter4 = (*iter2).first.begin();
				bool notEqual = false;
				for(auto iter3 = candidate.begin(); iter3 != --candidate.end(); iter3++) {
					if(*iter3 != * iter4) {
						notEqual = true;
						break;
					}
					iter4++;
				}
				if(notEqual) {
					continue;
				}


				candidate.insert(*(--iter2->first.end()));

				if(hasInfrequentSubset(candidate)) {
					continue;
				}

				candidates[candidate] = 0;
			}
		}

		return candidates;
	}

	bool hasInfrequentSubset(Itemset& candidate)
	{
		auto subsets = getSubsets(candidate.size()-1, candidate);

		for(auto& subset : subsets) {
			if(Ls[candidate.size()-1].count(subset) == 0) {
				return true;
			}
		}
		return false;
	}

	Itemsets getFrequent1Itemsets()
	{
		database.open();

		Itemsets itemsets;

		while(!database.isDone()) {
			Itemset itemset = database.nextTransaction();
			for(const ItemType& item : itemset) {
				auto ret = itemsets.insert(std::make_pair(Itemset{item}, 0));
				ret.first->second += 1;
			}

			numberOfTransactions++;
		}

		for(auto iter = itemsets.begin(); iter != itemsets.end(); ++iter) {
			if(iter->second < supportCount) {
				itemsets.erase(iter);
				continue;
			}
		}

		database.close();
		return itemsets;
	}

	size_t supportCount;
	size_t numberOfTransactions;
	Database<ItemType>& database;
	std::map<unsigned int, Itemsets> Ls;
};

template <class T>
class DatDatabase : public Database<T>
{
public:
	DatDatabase(std::string file)
		: file(file)
	{
	}

	virtual bool open()
	{
		close();

		ifs.open(file);
		return true;
	}

	virtual bool close()
	{
		ifs.close();
	}

	virtual bool isDone()
	{
		return !ifs.good();
	}

	virtual typename Database<T>::Itemset nextTransaction()
	{
		std::string line;
		getline(ifs, line);

		std::stringstream ss(line);
		typename Database<T>::Itemset itemset;

		while(ss.good()) {
			T item;
			ss >> item;
			itemset.insert(item);
		}

		return itemset;
	}

private:
	std::ifstream ifs;
	std::string file;
};

int main()
{
	DatDatabase<std::string> db("mushroom.dat");
	FP<std::string> fp(db, 3);
	fp.calculateFrequentItemsets();

	/*Apriori<std::string> apriori(db, 2);
	apriori.calculateFrequentItemsets();

	auto& frequentItemsets = apriori.getFrequentItemsets();
	for(auto& itemsets : frequentItemsets) {
		std::cout << "Frequent " << itemsets.first << "-itemsets" << std::endl;

		for(auto& itemset : itemsets.second) {
			for(auto& item : itemset.first) {
				std::cout << "'" << item << "' ";
			}
			std::cout << " : " << itemset.second << std::endl;
		}
		std::cout << "_____________________" << std::endl << std::endl;
	}*/

}
