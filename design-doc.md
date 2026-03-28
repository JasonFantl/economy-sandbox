

I want to build a 1D economy simulation in C using raylib and raygui. The simulation should handle thousands of agents who move along a horizontal line, work jobs (lumberjack, miner, blacksmith, carpenter, farmer), and trade goods with each other. There are multiple cities connected by travel depots. The user can pause the simulation, click an agent to inspect their internal state, interact with the environment (block trade routes, burn forests), and fast-forward the simulation. We will have many plots we can view, seeing the demand and supply, th eprice spead of a good, the wealth of agents, etc. These plots should be interactive, we can select different stats to view. Start by setting up the project structure and a basic raylib window with a 1D city layout and some agents moving along it. 

We are following the tutorials from https://jasonfantl.com/categories/simulated-economy/

To begin, just follow the 1st tutorial.

We will have a side-view of the city (just squares for houses), and a flat ground. For now agents (circles for now) move left and right along the ground moving towards a random other agent, then selecting a new agent to move to. Each time an agent meets the agent they were searching out, a trade is attempted. We should see a plot of the prices converge as agents attempt to trade (not yet actually using money).

