from sys import argv
import logging
import itertools

logging.basicConfig(level=logging.DEBUG)

def has_item(transaction, target):
    for item in transaction:
        if item == target:
            return True

def has_itemset(transaction, itemset):
    found = True
    for item in itemset:
        if not has_item(transaction, item):
            found = False
            break
    return found

def initialize_itemset_table(itemset_table, index):
    if index not in itemset_table:
        itemset_table[index] = {}

def increment_itemset(itemset_table, index, itemset):
    initialize_itemset_table(itemset_table, index)

    if itemset not in itemset_table[index]:
        itemset_table[index][itemset] = 1
    else:
        itemset_table[index][itemset] += 1

def filter_infrequent(itemset_table, min_support):
    infrequents = []
    for itemset,count in itemset_table.items():
        if count < min_support:

            infrequents.append(itemset)



    for itemset in infrequents:
        logging.debug('Removing infrequent itemset: %s, count: %d, min_support: %d', str(itemset), itemset_table[itemset], min_support)
        del itemset_table[itemset]

def natural_join(set1, set2):
    result = {}
    # Join on n-1 attributes
    i = 0
    for elem1 in set1:
        j = 0
        for elem2 in set2:
            if i < j:
                # use tmp because append does not return itself.
                tmp = list(elem1)
                tmp.append(elem2[-1])
                tmp = tuple(tmp)
                result[tmp] = 0
            j += 1

        i += 1

    return result

def has_infrequent_subset(itemset_table, itemset):
    subsets = itertools.combinations(itemset, len(itemset) - 1)
    for subset in subsets:
        if subset not in itemset_table:
            return True

    return False

def apriori(min_support):

    L = {}
    C = {}
    n_transactions = 0

    # Find Frequent-1 itemsets L1
    for transaction in read_database():
        print(transaction)
        for item in transaction:
            increment_itemset(C, 1, (item,))
        n_transactions += 1

    filter_infrequent(C[1], min_support)
    L[1] = C[1]

    k = 2
    while True:
        print('Starting k:', k)
        print('Previous k has:', len(L[k-1]), ' frequent items')

        # Join with self to get new candidates.
        candidates = natural_join(L[k-1], L[k-1])
        print('Generated', len(candidates), 'candidates')
        infrequents = []
        for candidate in candidates:
            if has_infrequent_subset(L[k-1], candidate):
                infrequents.append(candidate)

        for candidate in infrequents:
            del candidates[candidate]

        print('Infrequent subsets removed:', len(infrequents))

        C[k] = candidates

        n = 1
        for transaction in read_database():
            print('Checking transaction number:', n)
            for candidate in candidates:

                if has_itemset(transaction, candidate):
                    increment_itemset(C, k, candidate)

            n += 1
        filter_infrequent(C[k], min_support)
        if len(C[k]) == 0:
            break
        L[k] = C[k]

        k += 1

    return L

def subtract(set1, set2):
    result = []
    for elem in set1:
        if elem not in set2:
            result.append(elem)
    return tuple(result)

def findRules(L, min_conf):
    rules = []
    for k in range(1, len(L)+1):
        for itemset, count in L[k].items():
            # generate al subsets of itemset and add rule subset => itemset/subset with confidence > min_conf
            for i in range(1, len(itemset)):
                subsets = itertools.combinations(itemset, i)
                for subset in subsets:
                    confidence = L[k][itemset]/L[len(subset)][subset]
                    if confidence < min_conf:
                        continue
                    rules.append((subset, subtract(itemset, subset), confidence))

    return rules

def read_database():
    fh = open(argv[1])
    while True:
        line = fh.readline()
        line = line.rstrip("\n")
        if not line:
            break
        cells = line.split(",")
        for i in range(0, len(cells)):
            cells[i] = float(cells[i])
            if(cells[i] >= 0):
                cells[i] = 'joke-' + str(i) + '-funny'
            else:
                cells[i] = 'joke-' + str(i) + '-notfunny'
        yield tuple(cells)

        #yield tuple(line.split(",")[1:])
    fh.close()

def showL(L):
    for k in range(1,len(L)+1):
        print('Frequent', k, 'itemsets')
        for itemset,count in L[k].items():
            print(itemset, 'support =', count)
        print('----------------------')

def showRules(rules):
    print('Rules')
    for (lhs, rhs, confidence) in rules:
        print(lhs, '=>', rhs, 'with confidence', confidence)

L = apriori(3)
rules = findRules(L, 0.5)
showL(L)
showRules(rules)
