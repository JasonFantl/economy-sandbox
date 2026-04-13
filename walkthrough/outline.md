# Walkthrough:

Every chapter uses the same economy sim backend engine, we should be able to disable certain features so we can slowly introduce them.
Each section has a restart button. The scene has an initialization, and depending on which step in the scene we are different features my be on and plots shown.
Each step has a text box describing what is happening.
Player should be easily able to move to the next or previous step/section.
Scenes should be modular and self-contained.
Ends in an open game with all features turned on, which is where most time will be spent, this is just the introductory walkthrough.

## Chapter 1: One good

### Scene 1: Two agents
Two agents trading (no diminishing returns, no inflation, debt is allowed, just expected market price being updated after each attempt to trade) of single good, see both converge to a shared price. Show plot of base value and expected price (does not show marginal buy and sell prices) and the price history.

### Scene 2:
Lots of agents (100), we should see all converge. Uses the same plot from above.
#### Next
Show plot of supply and demand using the buy and sell price. Add equilibrium price to price plot, should see everyone converged to it. Allow player to reset the base values of agents between a min and max.
#### Next
We give agents a random number of goods and random money. They are obsessive, agents either spend all their money or sell all their goods. Show market collapsing as agents trade everything away.
#### Next
Introduce diminishing returns. Tell user to hit restart. Show market stabilizing. Plots of base value + buy and sell prices + expected market price. Show plot of supply and demand curves using the different buy and sell prices. Show plot of money vs good count.
#### Next
Goods break over time, show plots as wood breaks. Player can modify the break rate.
#### Next
Introduce the ability for agents to chop down wood, show plots as market is flooded. Player can modify the production rate. Tell the player to change the rates so one is higher then the other, can control if the market is being flooded or non-existent.
#### Next
Introduce leisure (a constant value, does not have diminishing returns). Should see the market stabilize. Player can also modify the leisure value, see how the market is effected.

## Chapter 2: Two goods

WIP

### Scene 1:
Multiple markets! Two goods, one can be converted into the other, agents choose now between three options: relax, chop wood, build chair.
Can explore a lot now: Are markets coupled in both directions? Effects of more efficient technology (produce more per unit, or build for less materials)? Inject lots of a good into the market just once, what are short term and long term effects of this shock? Change base value of a good (through a newspapers article or something). Case study on agent with extreme parameters (high base value, stubbornly refuses to change market rate).
Multiple cities. We kind of already have this. We can just physically split a group in two (with a wall for example) and we should see the economies separate (need some difference on one sid of the wall from the other).
Elasticity of a good: How does changing the price change the supply and demand? We can construct different demand-supply curves that show almost no change vs perfectly moving with the price change.
Substitutes: produce good C with 3A or 4B. Look at relationship of one market relative the other as we change variables.
Complimentary goods: TODO

