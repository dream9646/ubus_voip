#ifndef __ER_GROUP_HW_UTIL_H__
#define __ER_GROUP_HW_UTIL_H__

/* er_group_hw_util.c */
char *er_group_hw_util_action_str(int action);
int er_group_hw_util_mac_learning_depth_set(unsigned int port, unsigned int depth, unsigned int learning_ind);
int er_group_hw_util_mac_learning_depth_set2(unsigned int port, unsigned int port_depth, unsigned global_depth, unsigned int learning_ind);

#endif
