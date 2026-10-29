// BST Store backend. BST KEY = Product ID (primary key).
// Compile: g++ -std=c++17 -O2 -D_WIN32_WINNT=0x0A00 main.cpp -o server.exe -lws2_32
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

// ---------------- Product + BST node ----------------
struct Product {
    int id;               // BST key
    string name, category;
    double price;
    int quantity;
    double rating;
};

struct ProductNode {
    Product data;
    ProductNode *left, *right;   // left: smaller IDs, right: larger IDs
    ProductNode(const Product& p) : data(p), left(nullptr), right(nullptr) {}
};

// ---------------- helpers (JSON) ----------------
string esc(const string& s) {
    string o;
    for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
    return o;
}
string num(double v, int d) { ostringstream o; o << fixed << setprecision(d) << v; return o.str(); }
string pj(const Product& p) {
    return "{\"id\":" + to_string(p.id) + ",\"name\":\"" + esc(p.name) + "\",\"category\":\"" + esc(p.category) +
           "\",\"price\":" + num(p.price, 2) + ",\"quantity\":" + to_string(p.quantity) +
           ",\"rating\":" + num(p.rating, 1) + "}";
}
string listJson(const vector<Product>& v) {
    string s = "[";
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + pj(v[i]);
    return s + "]";
}
string intsJson(const vector<int>& v) {
    string s = "[";
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + to_string(v[i]);
    return s + "]";
}
string lower(string s) { transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }

// ---------------- BST ----------------
class BST {
    ProductNode* root = nullptr;

    void destroy(ProductNode* n) { if (n) { destroy(n->left); destroy(n->right); delete n; } }

    // Delete: 3 cases -> leaf, one child, two children (replace with inorder successor)
    ProductNode* deleteNode(ProductNode* n, int id, bool& ok) {
        if (!n) return nullptr;
        if (id < n->data.id) n->left = deleteNode(n->left, id, ok);
        else if (id > n->data.id) n->right = deleteNode(n->right, id, ok);
        else {
            ok = true;
            if (!n->left)  { ProductNode* r = n->right; delete n; return r; }  // 0 or 1 child
            if (!n->right) { ProductNode* l = n->left;  delete n; return l; }
            ProductNode* succ = findMin(n->right);          // smallest in right subtree
            n->data = succ->data;                            // copy successor's data here
            n->right = deleteNode(n->right, succ->data.id, ok); // remove successor node
        }
        return n;
    }
    // Inorder = Left, Node, Right -> sorted by Product ID
    void inorder(ProductNode* n, vector<Product>& out) {
        if (!n) return;
        inorder(n->left, out); out.push_back(n->data); inorder(n->right, out);
    }
    // Preorder traversal used for name search (tree is not ordered by name)
    void searchName(ProductNode* n, const string& q, vector<Product>& out, vector<int>& visited) {
        if (!n) return;
        visited.push_back(n->data.id);
        if (lower(n->data.name).find(q) != string::npos) out.push_back(n->data);
        searchName(n->left, q, out, visited);
        searchName(n->right, q, out, visited);
    }
    int height(ProductNode* n) { return n ? 1 + max(height(n->left), height(n->right)) : 0; }
    int countNodes(ProductNode* n) { return n ? 1 + countNodes(n->left) + countNodes(n->right) : 0; }
    string toJson(ProductNode* n) {
        if (!n) return "null";
        return "{\"id\":" + to_string(n->data.id) + ",\"name\":\"" + esc(n->data.name) + "\",\"left\":" +
               toJson(n->left) + ",\"right\":" + toJson(n->right) + "}";
    }
public:
    ~BST() { destroy(root); }

    // Insert (iterative). Walk down comparing IDs; attach at first empty spot.
    // path = IDs visited. Returns false if ID already exists (duplicate).
    bool insert(const Product& p, vector<int>& path) {
        if (!root) { root = new ProductNode(p); path.push_back(p.id); return true; }
        ProductNode* cur = root;
        while (true) {
            path.push_back(cur->data.id);
            if (p.id == cur->data.id) return false;
            ProductNode*& next = (p.id < cur->data.id) ? cur->left : cur->right;
            if (!next) { next = new ProductNode(p); path.push_back(p.id); return true; }
            cur = next;
        }
    }
    // Search by ID: go left if smaller, right if larger. comparisons = path.size()
    ProductNode* search(int id, vector<int>& path) {
        ProductNode* cur = root;
        while (cur) {
            path.push_back(cur->data.id);
            if (id == cur->data.id) return cur;
            cur = (id < cur->data.id) ? cur->left : cur->right;
        }
        return nullptr;
    }
    vector<Product> searchByName(const string& q, vector<int>& visited) {
        vector<Product> out; searchName(root, lower(q), out, visited); return out;
    }
    bool remove(int id) { bool ok = false; root = deleteNode(root, id, ok); return ok; }
    vector<Product> inorder() { vector<Product> v; inorder(root, v); return v; }
    ProductNode* findMin(ProductNode* n) { while (n && n->left) n = n->left; return n; }   // smallest ID
    ProductNode* findMax(ProductNode* n) { while (n && n->right) n = n->right; return n; } // largest ID
    ProductNode* getRoot() { return root; }
    int getHeight() { return height(root); }
    int count() { return countNodes(root); }
    string treeJson() { return toJson(root); }
    // Price range: traverse inorder and filter (BST is keyed by ID, so price needs a full traversal)
    vector<Product> priceRange(double lo, double hi) {
        vector<Product> all = inorder(), out;
        for (auto& p : all) if (p.price >= lo && p.price <= hi) out.push_back(p);
        return out;
    }
    // Min / max PRICE product (traversal, since price is not the key)
    bool priceExtremes(Product& mn, Product& mx) {
        vector<Product> all = inorder();
        if (all.empty()) return false;
        mn = mx = all[0];
        for (auto& p : all) { if (p.price < mn.price) mn = p; if (p.price > mx.price) mx = p; }
        return true;
    }
};

// ---------------- request parsing helpers ----------------
string getField(const string& b, const string& key) {   // tiny reader for flat JSON
    size_t p = b.find("\"" + key + "\"");
    if (p == string::npos) return "";
    p = b.find(':', p);
    if (p == string::npos) return "";
    p++;
    while (p < b.size() && isspace((unsigned char)b[p])) p++;
    if (p < b.size() && b[p] == '"') { size_t e = b.find('"', p + 1); return e == string::npos ? "" : b.substr(p + 1, e - p - 1); }
    size_t e = b.find_first_of(",}", p);
    string v = b.substr(p, e == string::npos ? string::npos : e - p);
    while (!v.empty() && isspace((unsigned char)v.back())) v.pop_back();
    return v;
}
bool toInt(const string& s, int& v) {
    try { size_t pos; v = stoi(s, &pos); return pos == s.size(); } catch (...) { return false; }
}
bool toDouble(const string& s, double& v) {
    try { size_t pos; v = stod(s, &pos); return pos == s.size(); } catch (...) { return false; }
}
void reply(httplib::Response& res, int status, const string& body) { res.status = status; res.set_content(body, "application/json"); }
void fail(httplib::Response& res, int status, const string& msg) {
    reply(res, status, "{\"success\":false,\"error\":\"" + esc(msg) + "\"}");
}

// ---------------- main ----------------
int main() {
    BST bst;
    int searches = 0, hits = 0, misses = 0;
    vector<int> tmp;
    // Sample data. Insertion order decides the tree shape (root = 105).
    vector<Product> seed = {
        {105, "Laptop", "Computers", 55999, 12, 4.6}, {101, "Smartphone", "Mobiles", 24999, 30, 4.5},
        {120, "Headphones", "Audio", 2499, 50, 4.3},  {100, "Keyboard", "Accessories", 1299, 40, 4.1},
        {103, "Mouse", "Accessories", 599, 80, 4.2},  {115, "Monitor", "Computers", 11999, 3, 4.4},
        {150, "Smart Watch", "Wearables", 3999, 25, 4.0}, {110, "SSD", "Storage", 4499, 0, 4.7},
        {118, "USB Drive", "Storage", 799, 100, 4.1}, {160, "Webcam", "Accessories", 1999, 15, 3.9}};
    for (auto& p : seed) { tmp.clear(); bst.insert(p, tmp); }

    httplib::Server svr;
    // CORS on every response + preflight
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                             {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
                             {"Access-Control-Allow-Headers", "Content-Type"}});
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) { res.status = 204; });
    svr.set_mount_point("/", ".");   // serves index.html from this folder at http://localhost:8080

    // GET /api/products  -> inorder traversal (sorted by ID)
    svr.Get("/api/products", [&](const httplib::Request&, httplib::Response& res) {
        auto v = bst.inorder();
        reply(res, 200, "{\"success\":true,\"count\":" + to_string(v.size()) + ",\"products\":" + listJson(v) + "}");
    });

    // GET /api/products/search?id=101  or  ?name=laptop
    svr.Get("/api/products/search", [&](const httplib::Request& req, httplib::Response& res) {
        vector<int> path; vector<Product> found; string type;
        if (req.has_param("id")) {
            type = "id"; int id;
            if (!toInt(req.get_param_value("id"), id) || id <= 0) return fail(res, 400, "Invalid product ID");
            ProductNode* n = bst.search(id, path);
            if (n) found.push_back(n->data);
        } else if (req.has_param("name")) {
            type = "name"; string q = req.get_param_value("name");
            if (q.empty()) return fail(res, 400, "Product name is empty");
            found = bst.searchByName(q, path);
        } else return fail(res, 400, "Use ?id= or ?name=");
        searches++; if (found.empty()) misses++; else hits++;
        reply(res, 200, string("{\"success\":true,\"found\":") + (found.empty() ? "false" : "true") + ",\"type\":\"" + type +
              "\",\"path\":" + intsJson(path) + ",\"comparisons\":" + to_string(path.size()) + ",\"products\":" + listJson(found) + "}");
    });

    // GET /api/products/range?min=1000&max=5000
    svr.Get("/api/products/range", [&](const httplib::Request& req, httplib::Response& res) {
        double lo, hi;
        if (!toDouble(req.get_param_value("min"), lo) || !toDouble(req.get_param_value("max"), hi) || lo > hi)
            return fail(res, 400, "Invalid price range");
        auto v = bst.priceRange(lo, hi);
        reply(res, 200, "{\"success\":true,\"count\":" + to_string(v.size()) + ",\"products\":" + listJson(v) + "}");
    });

    // GET /api/tree -> nested JSON for visualization
    svr.Get("/api/tree", [&](const httplib::Request&, httplib::Response& res) {
        reply(res, 200, "{\"success\":true,\"tree\":" + bst.treeJson() + "}");
    });

    // GET /api/stats -> dashboard
    svr.Get("/api/stats", [&](const httplib::Request&, httplib::Response& res) {
        Product mn, mx; bool has = bst.priceExtremes(mn, mx);
        reply(res, 200, "{\"success\":true,\"total\":" + to_string(bst.count()) + ",\"searches\":" + to_string(searches) +
              ",\"hits\":" + to_string(hits) + ",\"misses\":" + to_string(misses) + ",\"height\":" + to_string(bst.getHeight()) +
              ",\"minPrice\":" + (has ? num(mn.price, 2) : "null") + ",\"maxPrice\":" + (has ? num(mx.price, 2) : "null") + "}");
    });

    // POST /api/products  (JSON body)
    svr.Post("/api/products", [&](const httplib::Request& req, httplib::Response& res) {
        Product p;
        if (!toInt(getField(req.body, "id"), p.id) || p.id <= 0) return fail(res, 400, "Invalid product ID (positive integer required)");
        p.name = getField(req.body, "name"); p.category = getField(req.body, "category");
        if (p.name.empty()) return fail(res, 400, "Product name is required");
        if (p.category.empty()) return fail(res, 400, "Category is required");
        if (!toDouble(getField(req.body, "price"), p.price) || p.price <= 0) return fail(res, 400, "Invalid price (must be > 0)");
        if (!toInt(getField(req.body, "quantity"), p.quantity) || p.quantity < 0) return fail(res, 400, "Invalid quantity (must be >= 0)");
        if (!toDouble(getField(req.body, "rating"), p.rating) || p.rating < 0 || p.rating > 5) return fail(res, 400, "Invalid rating (0 to 5)");
        vector<int> path;
        if (!bst.insert(p, path)) return fail(res, 409, "Duplicate product ID");
        reply(res, 201, "{\"success\":true,\"message\":\"Product inserted\",\"path\":" + intsJson(path) + ",\"product\":" + pj(p) + "}");
    });

    // DELETE /api/products/101
    svr.Delete(R"(/api/products/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        int id;
        if (!toInt(req.matches[1], id) || id <= 0) return fail(res, 400, "Invalid product ID");
        if (!bst.remove(id)) return fail(res, 404, "Product not found");
        reply(res, 200, "{\"success\":true,\"message\":\"Product " + to_string(id) + " deleted\"}");
    });

    cout << "BST Store running at http://localhost:8080  (Ctrl+C to stop)\n";
    if (!svr.listen("0.0.0.0", 8080)) cout << "Could not start server (port 8080 busy?)\n";
    return 0;
}
