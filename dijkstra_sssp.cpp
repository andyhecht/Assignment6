#include <cstdio>
#include <climits>
#include <time.h>
#include <vector>
#include <queue>

using namespace std;

int *dijkstra(int src, vector<vector<pair<int, int> > >& u) {
	int n = u.size(), vis[n], *dist = new int[n], node_relax = 0;

	for (int i = 0; i < n; i++) {
		dist[i] = INT_MAX;
		vis[i] = 0;
	}

	class priority {public: bool operator()(pair<int, int>& p1, pair<int, int>& p2)
		{return p1.second > p2.second;}};
	priority_queue<pair<int, int>, vector<pair<int, int> >, priority> pq;

	dist[src] = 0;
	pq.push(make_pair(src, 0));

	while (!pq.empty()) {
		auto curr = pq.top();
		pq.pop();
		int cv = curr.first, cw = curr.second;

		if (vis[cv]) continue;
		vis[cv] = 1;
		node_relax++;
		for (size_t v = 0; v < u[cv].size(); v++) {
			int nv = u[cv][v].first;
			int nw = u[cv][v].second + cw;
			if (!vis[nv] && nw < dist[nv]) {
				dist[nv] = nw;
				pq.push(make_pair(nv, nw));
			}
		}
	}

	printf("Node Relaxations: %d\n", node_relax);

	return dist;
}

int main()
{
	int n, m, a, b, w;

	FILE *fp = fopen("rmat15.dimacs", "r");

	struct timespec tick, tock;

	fscanf(fp, "p sp %d %d ", &n, &m);
	vector<vector<pair<int, int> > > u(n);
	bool *foo = new bool[n]();
	int count = 0;
	for (int i = 0; i < m; i++) {
		fscanf(fp, "a %d %d %d ", &a, &b, &w);
		if (a == b) continue;
		u[a-1].push_back(make_pair(b-1, w));
		foo[a-1]=true;
		foo[b-1]=true;
	}
	for (int i = 0; i < n; i++)
		if (foo[i]) count++;
	printf("count: %d\n", count);
	delete []foo;

	fclose(fp);
	FILE *fi = fopen("results1.out", "w");

	clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
	int *res = dijkstra(0, u);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tock);

	unsigned long long int exec_time = 1000000000 * (tock.tv_sec - tick.tv_sec) +
		tock.tv_nsec - tick.tv_nsec;

	fprintf(fi, "Dijkstra Results\nElapsed Time: %llu ns\n", exec_time);
	for (int i = 0; i < n; i++)
		fprintf(fi, "%d %d\n", i+1, res[i]);

	fclose(fi);
	delete []res;

	return 0;
}


