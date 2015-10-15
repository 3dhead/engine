require "ai.shared"

function rabbitStayAlive (parentnode)
	parentnode:addNode("Steer(SelectionFlee)", "fleefromhunter"):setCondition("And(Filter(SelectNpcsOfTypes{ANIMAL_WOLF}),IsCloseToSelection{10})")
end

function rabbit ()
	local name = "ANIMAL_RABBIT"
	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
	rabbitStayAlive(rootNode)
	increasePopulation(rootNode)
	idle(rootNode)
	die(rootNode)
end
