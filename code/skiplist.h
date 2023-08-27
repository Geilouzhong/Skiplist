#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;
std::string delimiter = ":";

// 跳表节点
template<typename K, typename V> 
class Node {
public:
    Node() {} 
    Node(K k, V v, int); 
    ~Node();
    K get_key() const;
    V get_value() const;
    void set_value(V);
    
    // 指针数组，forward[i]指向当前节点第i级索引的下一个节点
    Node<K, V> **forward;
    int node_level;

private:
    K key_;
    V value_;
};

template<typename K, typename V> 
Node<K, V>::Node(const K k, const V v, int level) {
    this->key_ = k;
    this->value_ = v;
    this->node_level = level; 

    // 索引等级范围 0 - level
    this->forward = new Node<K, V>*[level+1];
    
	// 初始化
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

template<typename K, typename V> 
Node<K, V>::~Node() {
    delete []forward;
};

template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key_;
};

template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value_;
};
template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value_=value;
};


template <typename K, typename V> 
class SkipList {

public: 
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K, V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    void load_file();
    int size();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:    
    // 最大索引等级上限
    int _max_level;

    // 当前跳表最大索引等级
    int _skip_list_level;

    Node<K, V> *_header;

    // 文件操作
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // 当前跳表节点数
    int _element_count;
};

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}


// 插入元素
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    
    mtx.lock();
    Node<K, V> *current = this->_header;

    // updated记录插入位置的各级索引的前一个Node
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  

    // 从最高级索引开始寻找插入位置 
    for(int i = _skip_list_level; i >= 0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i]; 
        }
        update[i] = current;
    }

    current = current->forward[0];

    // 待插入的key已存在
    if (current != NULL && current->get_key() == key) {
        // std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // 插入位置在跳表尾 或 插入位置在update[0]和current之间
    if (current == NULL || current->get_key() != key ) {
        
        // 随机生成索引等级
        int random_level = get_random_level();

        // 随机生成的等级比当前跳表最大索引等级高，补充update数组，更新跳表最大等级
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        Node<K, V>* inserted_node = create_node(key, value, random_level);
        
        // 逐层插入
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        // std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;  // 更新节点数
    }
    mtx.unlock();
    return 0;
}

template<typename K, typename V> 
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = _skip_list_level; i >= 0; i--) {
        Node<K, V> *node = this->_header->forward[i]; 
        std::cout << "[Level " << i << "] ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << "; ";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 数据落盘
template<typename K, typename V> 
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V> *node = this->_header->forward[0]; 

    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

// 文件加载数据
template<typename K, typename V> 
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        // std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    delete key;
    delete value;
    _file_reader.close();
}

// 获取当前跳表节点数
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

// 从str中获取key和value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

// 删除元素
template<typename K, typename V> 
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();
    Node<K, V> *current = this->_header; 
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    // 从最高级索引开始寻找删除位置的前一个节点，用update记录
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
       
        // 从最低层开始逐层删除当前节点
        for (int i = 0; i <= _skip_list_level; i++) {

            // 如果update[i]的下一个节点不是待删除节点则停止循环
            if (update[i]->forward[i] != current) 
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // 删除没有元素的索引等级
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level --; 
        }

        // std::cout << "Successfully deleted key "<< key << std::endl;
        delete current;
        _element_count --;
    }
    mtx.unlock();
    return;
}

template<typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {

    // std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;

    // 从最高级索引开始查询
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    // 查询命中
    if (current and current->get_key() == key) {
        // std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    // std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

template<typename K, typename V> 
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // 创建头节点，初始化k，v为Null
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V> 
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    Node<K, V> *current = _header;
    Node<K, V> *tmp;
    while (current != NULL) {
        tmp = current;
        current = current->forward[0];
        delete tmp;
    }
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};

#endif