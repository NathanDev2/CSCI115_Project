#include <iostream>
#include <fstream>
#include <string>
#include <climits>
#include <cstdlib>
#include <conio.h>
using namespace std;

const char ENEMY = 'E', PLAYER = 'P', PATH = 'T', WALL = 'W', DIRT = 'D', SECRET_PASSAGE = 'S';
const int ENEMY_STOP_DISTANCE = 2;
const int MAX_ROWS = 10, MAX_COLS = 10, MAX_VERTICES = MAX_ROWS * MAX_COLS;
const string COLOR_PLAYER = "\033[32m", COLOR_ENEMY = "\033[31m", COLOR_DIRT = "\033[38;5;94m";  // Brown
const string COLOR_PATH = "\033[97m", COLOR_SECRET = "\033[35m", COLOR_WALL = "\033[38;5;214m", COLOR_RESET = "\033[0m";

class Array {
public:
    int* array;
    int arraySize;
    int capacity;
    Array(int initialSize = 10);
    ~Array();
    void Insert(int x);
    int GetElement(int index) const;
    void Clear();
};

Array::Array(int initialSize) {
    arraySize = 0;
    capacity = initialSize;
    array = new int[capacity];
}
Array::~Array() { delete[] array; }
void Array::Insert(int x) {
    if (arraySize == capacity) {
        capacity *= 2;
        int* newArray = new int[capacity];
        for (int i = 0; i < arraySize; i++) newArray[i] = array[i];
        delete[] array;
        array = newArray;
    }
    array[arraySize++] = x;
}
int Array::GetElement(int index) const { return index >= 0 && index < arraySize ? array[index] : -1; }
void Array::Clear() { arraySize = 0; }

class Graph {
public:
    int adjMatrix[MAX_VERTICES][MAX_VERTICES];
    int vertices;
    Graph(int v);
    void addEdge(int u, int v, int weight);
    Array Dijkstra(int start, int end);
};

Graph::Graph(int v) : vertices(v) {
    for (int i = 0; i < v; i++)
        for (int j = 0; j < v; j++)
            adjMatrix[i][j] = i == j ? 0 : INT_MAX;
}

void Graph::addEdge(int u, int v, int weight) {
    adjMatrix[u][v] = weight;
    adjMatrix[v][u] = weight;
}

Array Graph::Dijkstra(int start, int end) {
    int dist[MAX_VERTICES], prev[MAX_VERTICES];
    bool visited[MAX_VERTICES] = {false};
    for (int i = 0; i < vertices; i++) dist[i] = INT_MAX, prev[i] = -1;
    dist[start] = 0;
    for (int i = 0; i < vertices; i++) {
        int u = -1;
        for (int j = 0; j < vertices; j++)
            if (!visited[j] && (u == -1 || dist[j] < dist[u]))
                u = j;
        if (dist[u] == INT_MAX) break;
        visited[u] = true;
        for (int v = 0; v < vertices; v++) {
            if (adjMatrix[u][v] != INT_MAX && dist[u] + adjMatrix[u][v] < dist[v]) {
                dist[v] = dist[u] + adjMatrix[u][v];
                prev[v] = u;
            }
        }
    }
    Array path;
    for (int at = end; at != -1; at = prev[at]) path.Insert(at);
    return path;
}

bool loadTerrainFromFile(char terrain[MAX_ROWS][MAX_COLS], Graph &graph, int &playerVertex, Array &enemyPositions, int &rows, int &cols, const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        return false;
    }
    string line;
    rows = 0;
    cols = 0;
    while (getline(file, line) && rows < MAX_ROWS) {
        cols = max(cols, static_cast<int>(line.length()));
        for (size_t col = 0; col < line.length() && col < MAX_COLS; col++) {
            terrain[rows][col] = line[col];
            int position = rows * MAX_COLS + col;
            if (terrain[rows][col] == PLAYER) playerVertex = position;
            else if (terrain[rows][col] == ENEMY) enemyPositions.Insert(position);
            if (terrain[rows][col] == PATH || terrain[rows][col] == DIRT || terrain[rows][col] == SECRET_PASSAGE || terrain[rows][col] == PLAYER || terrain[rows][col] == ENEMY) {
                int weight = terrain[rows][col] == DIRT ? 2 : 1;
                if (rows > 0 && terrain[rows-1][col] != WALL) graph.addEdge(position, (rows - 1) * MAX_COLS + col, weight);
                if (rows < MAX_ROWS - 1 && terrain[rows+1][col] != WALL) graph.addEdge(position, (rows + 1) * MAX_COLS + col, weight);
                if (col > 0 && terrain[rows][col-1] != WALL) graph.addEdge(position, rows * MAX_COLS + (col - 1), weight);
                if (col < MAX_COLS - 1 && terrain[rows][col+1] != WALL) graph.addEdge(position, rows * MAX_COLS + (col + 1), weight);
            }
        }
        rows++;
    }
    file.close();
    return true;
}

void displayTerrain(char terrain[MAX_ROWS][MAX_COLS], int rows, int cols) {
    system("cls"); // Use "clear" on Linux/Mac
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (terrain[i][j] == PLAYER) cout << COLOR_PLAYER << "o " << COLOR_RESET;
            else if (terrain[i][j] == ENEMY) cout << COLOR_ENEMY << "x " << COLOR_RESET;
            else if (terrain[i][j] == DIRT) cout << COLOR_DIRT << "* " << COLOR_RESET;
            else if (terrain[i][j] == PATH) cout << COLOR_PATH << ". " << COLOR_RESET;
            else if (terrain[i][j] == SECRET_PASSAGE) cout << COLOR_SECRET << "* " << COLOR_RESET;
            else if (terrain[i][j] == WALL) cout << COLOR_WALL << "# " << COLOR_RESET;
            else cout << "? ";
        }
        cout << endl;
    }
}

void movePlayer(char terrain[MAX_ROWS][MAX_COLS], int &playerRow, int &playerCol, char &lastTile) {
    char input;
    cout << "Enter W (up), A (left), S (down), D (right), or Q to quit: ";
    input = _getch();
    int newRow = playerRow, newCol = playerCol;
    if (input == 'W' || input == 'w') newRow--;
    else if (input == 'S' || input == 's') newRow++;
    else if (input == 'A' || input == 'a') newCol--;
    else if (input == 'D' || input == 'd') newCol++;
    else if (input == 'Q' || input == 'q') exit(0);

    // Ensure new position is within bounds and is walkable
    if (newRow >= 0 && newRow < MAX_ROWS && newCol >= 0 && newCol < MAX_COLS &&
        (terrain[newRow][newCol] == PATH || terrain[newRow][newCol] == SECRET_PASSAGE || terrain[newRow][newCol] == DIRT)) {
        // Restore the last tile's character where the player was
        terrain[playerRow][playerCol] = lastTile;
        
        // Update the last tile to the new position's tile type
        lastTile = terrain[newRow][newCol];
        
        // Move player to the new position
        terrain[newRow][newCol] = PLAYER;
        playerRow = newRow;
        playerCol = newCol;
    }
}

void moveEnemies(char terrain[MAX_ROWS][MAX_COLS], Graph &graph, int playerVertex, Array &enemyPositions) {
    for (int i = 0; i < enemyPositions.arraySize; i++) {
        int enemyVertex = enemyPositions.GetElement(i);
        Array path = graph.Dijkstra(enemyVertex, playerVertex);
        if (path.arraySize > ENEMY_STOP_DISTANCE + 1) {
            int nextMoveVertex = path.GetElement(path.arraySize - ENEMY_STOP_DISTANCE - 1);
            int enemyRow = enemyVertex / MAX_COLS;
            int enemyCol = enemyVertex % MAX_COLS;
            int newRow = nextMoveVertex / MAX_COLS;
            int newCol = nextMoveVertex % MAX_COLS;
            terrain[enemyRow][enemyCol] = PATH;
            terrain[newRow][newCol] = ENEMY;
            enemyPositions.array[i] = nextMoveVertex;
        }
    }
}

int main() {
    char terrain[MAX_ROWS][MAX_COLS];
    int rows, cols;
    int playerVertex;
    Array enemyPositions;
    Graph graph(MAX_VERTICES);

    if (!loadTerrainFromFile(terrain, graph, playerVertex, enemyPositions, rows, cols, "map1.txt")) return 1;
    int playerRow = playerVertex / MAX_COLS, playerCol = playerVertex % MAX_COLS;
    char lastTile = PATH;

    while (true) {
        displayTerrain(terrain, rows, cols);
        movePlayer(terrain, playerRow, playerCol, lastTile);
        playerVertex = playerRow * MAX_COLS + playerCol;
        moveEnemies(terrain, graph, playerVertex, enemyPositions);
    }
    return 0;
}
