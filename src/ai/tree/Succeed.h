#pragma once

#include "tree/TreeNode.h"
#include "common/Types.h"
#include "AIRegistry.h"

namespace ai {

/**
 * @brief A decorator node with only one child attached. The result of the attached child is only
 * taken into account if it returned @c TreeNodeStatus::RUNNING - in every other case this decorator
 * will return @c TreeNodeStatus::FINISHED.
 */
class Succeed: public TreeNode {
public:
	NODE_CLASS(Succeed)

	TreeNodeStatus execute(AI& entity, long deltaMillis) override {
		if (_children.size() != 1) {
			ai_assert(false, "Succeed can not have more than one child");
		}

		if (TreeNode::execute(entity, deltaMillis) == CANNOTEXECUTE)
			return CANNOTEXECUTE;

		const TreeNodePtr& treeNode = *_children.begin();
		const TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
		if (status == RUNNING)
			return state(entity, RUNNING);
		return state(entity, FINISHED);
	}
};

}