# BST Store: Online Store Product Searching Using Binary Search Tree

**BST key = Product ID** (primary key). Left subtree has smaller IDs, right subtree has larger IDs.
Product name and price are NOT keys, so name search and price range need a traversal.

## 1. Architecture
Browser (index.html, style.css, script.js) --fetch (JSON)--> C++ server (main.cpp, cpp-httplib) --> BST in memory.
The server also serves the frontend files, so everything runs at http://localhost:8080. CORS headers are added too.

## 2. Software needed
- Windows 11, VS Code
- g++ (MinGW-w64 via MSYS2: https://www.msys2.org, then in the MSYS2 UCRT64 terminal run `pacman -S mingw-w64-ucrt-x86_64-gcc`, and add `C:\msys64\ucrt64\bin` to PATH)
- cpp-httplib (single header file)

## 3. Folder structure
```
bst-store/
├── main.cpp     (Product, ProductNode, BST class, HTTP API)
├── httplib.h    (download, see below)
├── index.html
├── style.css
└── script.js
```

## 4. Setup and compile (VS Code terminal, PowerShell)
```
cd path\to\bst-store
curl.exe -L -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
g++ -std=c++17 -O2 -D_WIN32_WINNT=0x0A00 main.cpp -o server.exe -lws2_32
.\server.exe
```
Compiling takes about 30 to 60 seconds because httplib.h is large. Allow the Windows firewall prompt if it appears.

## 5. Open the frontend
Go to **http://localhost:8080** in your browser (keep the server window open).

## 6. How frontend and backend talk
| Action | JS call | C++ route | BST function |
|---|---|---|---|
| Load products | GET /api/products | svr.Get | inorder() |
| Search ID | GET /api/products/search?id=101 | svr.Get | search() |
| Search name | GET /api/products/search?name=laptop | svr.Get | searchByName() |
| Add | POST /api/products (JSON) | svr.Post | insert() |
| Delete | DELETE /api/products/101 | svr.Delete | remove() / deleteNode() |
| Price range | GET /api/products/range?min=1000&max=5000 | svr.Get | priceRange() |
| Tree drawing | GET /api/tree | svr.Get | treeJson() |
| Dashboard | GET /api/stats | svr.Get | count(), getHeight(), priceExtremes() |

Search and insert responses include `path` (IDs visited) and `comparisons`; the frontend highlights those nodes.

## 7. Test every API (PowerShell)
```
curl.exe http://localhost:8080/api/products
curl.exe "http://localhost:8080/api/products/search?id=120"
curl.exe "http://localhost:8080/api/products/search?name=laptop"
curl.exe "http://localhost:8080/api/products/range?min=1000&max=5000"
curl.exe http://localhost:8080/api/tree
curl.exe http://localhost:8080/api/stats
curl.exe -X POST http://localhost:8080/api/products -H "Content-Type: application/json" -d "{\"id\":112,\"name\":\"Tablet\",\"category\":\"Mobiles\",\"price\":19999,\"quantity\":8,\"rating\":4.2}"
curl.exe -X DELETE http://localhost:8080/api/products/112
```
Error tests: duplicate ID (409), price -5 (400), rating 9 (400), id=abc (400), delete 999 (404).

## 8. BST implementation explained
- `ProductNode` holds a `Product` plus `left` and `right` pointers.
- **insert**: loop from root, go left/right by ID, attach at the first null pointer, reject duplicates.
- **search**: same walk; return the node on equal ID. Visited IDs are saved as the search path.
- **deleteNode**: leaf, one child, or two children (copy inorder successor, then delete it).
- **inorder**: Left, Node, Right gives sorted IDs.
- **height**: 1 + max(height(left), height(right)). **countNodes**: 1 + left + right.
- **findMin / findMax**: go all the way left / right (smallest / largest ID). Min/max price uses traversal.
- Sample tree (insert order 105,101,120,100,103,115,150,110,118,160) has root 105 and height 4.

## 9. Viva questions
1. **Why BST for product search?** Each comparison discards half the remaining tree on average, so search is O(log n) instead of O(n) for a list.
2. **What is the key?** Product ID, because it is unique and ordered.
3. **Why is name search slow?** The tree is ordered by ID, so name matches can be anywhere. We visit all nodes, O(n). A second BST keyed by name would fix this.
4. **Worst case?** Inserting sorted IDs makes a skewed tree (like a linked list), height n, so O(n).
5. **How to fix it?** Self-balancing trees: AVL or Red-Black tree.
6. **Delete with two children?** Replace with inorder successor (or predecessor), then delete that node.
7. **Which traversal gives sorted output?** Inorder.
8. **Why can't price range prune the tree?** Price is not the key, so subtrees are not ordered by price.
9. **Duplicates?** Rejected with HTTP 409.
10. **Inorder successor?** The smallest node in the right subtree.

## 10. Possible improvements
AVL balancing with rotation animation, a second BST by name, save and load products from a file, edit product, login for admin, pagination, unit tests.
