/**
 * @file interlock.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Laser safety interlock system interface.
 * 
 * This module provides laser safety interlock functionality to ensure safe
 * operation of laser systems. It monitors interlock conditions and can
 * automatically disable lasers when safety conditions are not met.
 * 
 * @version \projectnumber
 */

#pragma once

/**
 * @brief Initialize the laser interlock system.
 * 
 * Sets up the interlock timer/counter, configures monitoring pins,
 * and enables interlock detection interrupts.
 */
void init_interlock();

/**
 * @brief Global laser enable/disable state.
 * 
 * Controls whether lasers are currently enabled. This can be modified
 * by the interlock system or manually by the user.
 */
extern volatile bool lasers_enabled;

/**
 * @brief Interlock system enabled state.
 * 
 * Controls whether the interlock system is active and monitoring
 * safety conditions.
 */
extern bool interlock_enabled;
