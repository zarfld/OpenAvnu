/**
 * OpenAvnu Profile Framework - Dynamic Profile Switching Example
 * 
 * This example demonstrates how the new specification/version abstraction
 * enables runtime profile switching and compatibility checking without
 * hardcoded flags.
 */

#include "openavb_profile_framework.h"
#include <stdio.h>
#include <string.h>

/**
 * Example: Dynamic Profile Selection and Switching
 * Shows how applications can switch between profile versions at runtime
 */

// Profile selection context
typedef struct profile_selection_context {
    openavb_profile_cfg_t *current_profile;
    openavb_profile_cfg_t *fallback_profile;
    bool auto_fallback_enabled;
    U32 compatibility_level;
} profile_selection_context_t;

/**
 * Select the best available profile based on requirements and peer capabilities
 */
openavb_profile_cfg_t *select_optimal_profile(
    const char *preferred_profile_name,
    openavb_spec_version_e min_spec_version,
    openavb_spec_variant_e preferred_variant,
    const openavb_capability_matrix_t *required_capabilities)
{
    openavb_profile_cfg_t **profiles = openavb_profile_get_builtin_profiles();
    openavb_profile_cfg_t *best_match = NULL;
    int best_score = -1;
    
    printf("Profile Selection: Looking for '%s' with min version %d, variant %d\n",
           preferred_profile_name, min_spec_version, preferred_variant);
    
    for (int i = 0; profiles[i] != NULL; i++) {
        openavb_profile_cfg_t *profile = profiles[i];
        int score = 0;
        
        // Check basic compatibility
        if (profile->spec_version < min_spec_version) {
            printf("  Skipping %s: version %d < minimum %d\n",
                   profile->version_string, profile->spec_version, min_spec_version);
            continue;
        }
        
        // Score based on name match
        if (strcmp(profile->profile_name, preferred_profile_name) == 0) {
            score += 100;
        }
        
        // Score based on variant preference
        if (profile->spec_variant == preferred_variant) {
            score += 50;
        }
        
        // Score based on version (newer is generally better)
        score += (profile->spec_version - min_spec_version) * 10;
        
        // Check capability compatibility
        if (required_capabilities) {
            // Transport capabilities
            if (required_capabilities->transport.fast_connect_supported &&
                !profile->capabilities.transport.fast_connect_supported) {
                score -= 20;
            }
            
            if (required_capabilities->transport.secure_channels_supported &&
                !profile->capabilities.transport.secure_channels_supported) {
                score -= 30;
            }
            
            // Security capabilities
            if (required_capabilities->security.authentication_required &&
                !profile->capabilities.security.authentication_required) {
                score -= 40;  // Major penalty for missing security
            }
            
            // Timing capabilities
            if (required_capabilities->timing.min_presentation_offset_ns <
                profile->capabilities.timing.min_presentation_offset_ns) {
                score -= 10;  // Penalty for insufficient timing precision
            }
        }
        
        printf("  Profile %s scored %d points\n", profile->version_string, score);
        
        if (score > best_score) {
            best_score = score;
            best_match = profile;
        }
    }
    
    if (best_match) {
        printf("Selected profile: %s (score: %d)\n", best_match->version_string, best_score);
    } else {
        printf("No compatible profile found!\n");
    }
    
    return best_match;
}

/**
 * Check if a profile migration is possible and beneficial
 */
bool can_migrate_profile(openavb_profile_cfg_t *from_profile, 
                        openavb_profile_cfg_t *to_profile,
                        bool *requires_stream_restart)
{
    *requires_stream_restart = false;
    
    printf("Checking migration from %s to %s\n",
           from_profile->version_string, to_profile->version_string);
    
    // Check if profiles are compatible
    if (!openavb_profile_is_compatible(from_profile, to_profile)) {
        printf("  Migration not possible: profiles are incompatible\n");
        return false;
    }
    
    // Check if migration requires stream restart
    if (from_profile->capabilities.timing.max_frame_size != 
        to_profile->capabilities.timing.max_frame_size) {
        *requires_stream_restart = true;
        printf("  Migration requires stream restart: frame size changed\n");
    }
    
    if (from_profile->capabilities.security.authentication_required != 
        to_profile->capabilities.security.authentication_required) {
        *requires_stream_restart = true;
        printf("  Migration requires stream restart: security requirements changed\n");
    }
    
    // Check for capability improvements
    bool has_improvements = false;
    
    if (to_profile->capabilities.transport.fast_connect_supported &&
        !from_profile->capabilities.transport.fast_connect_supported) {
        has_improvements = true;
        printf("  Migration benefit: fast connect capability added\n");
    }
    
    if (to_profile->capabilities.timing.sync_uncertainty_tolerance_ns <
        from_profile->capabilities.timing.sync_uncertainty_tolerance_ns) {
        has_improvements = true;
        printf("  Migration benefit: improved timing precision\n");
    }
    
    if (to_profile->capabilities.qos.frame_preemption_supported &&
        !from_profile->capabilities.qos.frame_preemption_supported) {
        has_improvements = true;
        printf("  Migration benefit: frame preemption support added\n");
    }
    
    printf("  Migration %s: restart=%s, improvements=%s\n",
           "possible", *requires_stream_restart ? "yes" : "no",
           has_improvements ? "yes" : "no");
    
    return true;
}

/**
 * Dynamically switch between profile versions based on network conditions
 */
bool dynamic_profile_adaptation(profile_selection_context_t *context,
                               const openavb_stream_quality_metrics_t *metrics)
{
    printf("\nDynamic Profile Adaptation Analysis:\n");
    printf("Current profile: %s\n", context->current_profile->version_string);
    printf("Stream metrics: jitter=%.3fms, timing_violations=%u, late_packets=%u\n",
           metrics->jitter_ms, metrics->timing_violations, metrics->late_packets);
    
    // Check if current profile requirements are being met
    bool profile_satisfied = true;
    
    // Check timing requirements
    U32 timing_tolerance_ns = context->current_profile->capabilities.timing.sync_uncertainty_tolerance_ns;
    if (metrics->jitter_ms > (timing_tolerance_ns / 1000000.0)) {
        printf("  Issue: Jitter %.3fms exceeds tolerance %.3fms\n",
               metrics->jitter_ms, timing_tolerance_ns / 1000000.0);
        profile_satisfied = false;
    }
    
    // Check for timing violations
    if (metrics->timing_violations > 0) {
        printf("  Issue: %u timing violations detected\n", metrics->timing_violations);
        profile_satisfied = false;
    }
    
    // Check packet loss
    if (metrics->packets_lost > 0) {
        printf("  Issue: %u packets lost\n", metrics->packets_lost);
        profile_satisfied = false;
    }
    
    if (profile_satisfied) {
        printf("  Current profile requirements are satisfied\n");
        
        // Check if we can upgrade to a better profile
        openavb_profile_cfg_t **profiles = openavb_profile_get_builtin_profiles();
        for (int i = 0; profiles[i] != NULL; i++) {
            openavb_profile_cfg_t *candidate = profiles[i];
            
            // Look for profiles with same base but better capabilities
            if (strcmp(candidate->profile_name, context->current_profile->profile_name) == 0 &&
                candidate->spec_version > context->current_profile->spec_version) {
                
                bool requires_restart;
                if (can_migrate_profile(context->current_profile, candidate, &requires_restart)) {
                    printf("  Upgrade opportunity: %s (restart required: %s)\n",
                           candidate->version_string, requires_restart ? "yes" : "no");
                    
                    if (!requires_restart) {
                        printf("  Performing seamless upgrade to %s\n", candidate->version_string);
                        context->current_profile = candidate;
                        return true;
                    }
                }
            }
        }
        
        return false;  // No changes needed
    }
    
    // Profile requirements not satisfied - need to adapt
    printf("  Profile adaptation required\n");
    
    if (context->auto_fallback_enabled && context->fallback_profile) {
        bool requires_restart;
        if (can_migrate_profile(context->current_profile, context->fallback_profile, &requires_restart)) {
            printf("  Falling back to %s (restart required: %s)\n",
                   context->fallback_profile->version_string, requires_restart ? "yes" : "no");
            context->current_profile = context->fallback_profile;
            return true;
        }
    }
    
    // Look for a more lenient profile variant
    openavb_profile_cfg_t **profiles = openavb_profile_get_builtin_profiles();
    for (int i = 0; profiles[i] != NULL; i++) {
        openavb_profile_cfg_t *candidate = profiles[i];
        
        // Look for same profile with more lenient variant
        if (strcmp(candidate->profile_name, context->current_profile->profile_name) == 0 &&
            candidate->spec_version == context->current_profile->spec_version &&
            candidate->spec_variant != context->current_profile->spec_variant) {
            
            // Check if this variant has more lenient requirements
            if (candidate->capabilities.timing.sync_uncertainty_tolerance_ns >
                context->current_profile->capabilities.timing.sync_uncertainty_tolerance_ns) {
                
                bool requires_restart;
                if (can_migrate_profile(context->current_profile, candidate, &requires_restart)) {
                    printf("  Switching to more lenient variant: %s\n", candidate->version_string);
                    context->current_profile = candidate;
                    return true;
                }
            }
        }
    }
    
    printf("  No suitable profile adaptation found\n");
    return false;
}

/**
 * Example usage of the dynamic profile system
 */
void demonstrate_profile_abstraction(void)
{
    printf("=== OpenAvnu Profile Framework Abstraction Demo ===\n\n");
    
    // Example 1: Profile selection based on requirements
    printf("1. Dynamic Profile Selection\n");
    printf("============================\n");
    
    openavb_capability_matrix_t requirements = {
        .transport = {
            .fast_connect_supported = true,
            .secure_channels_supported = false
        },
        .security = {
            .authentication_required = false
        },
        .timing = {
            .min_presentation_offset_ns = 500000  // 500µs
        }
    };
    
    openavb_profile_cfg_t *selected = select_optimal_profile(
        "MILAN", 
        OPENAVB_SPEC_MILAN_1_0,
        OPENAVB_VARIANT_STRICT,
        &requirements
    );
    
    if (selected) {
        printf("Successfully selected: %s\n", selected->version_string);
        printf("Capabilities: fast_connect=%s, secure_channels=%s\n",
               selected->capabilities.transport.fast_connect_supported ? "yes" : "no",
               selected->capabilities.transport.secure_channels_supported ? "yes" : "no");
    }
    
    // Example 2: Dynamic profile adaptation
    printf("\n2. Dynamic Profile Adaptation\n");
    printf("==============================\n");
    
    profile_selection_context_t context = {
        .current_profile = selected,
        .fallback_profile = openavb_profile_get_by_version(OPENAVB_SPEC_IEEE_1722_2016, OPENAVB_VARIANT_STANDARD),
        .auto_fallback_enabled = true,
        .compatibility_level = 1
    };
    
    // Simulate poor network conditions
    openavb_stream_quality_metrics_t poor_metrics = {
        .packets_sent = 1000,
        .packets_lost = 5,
        .late_packets = 3,
        .timing_violations = 2,
        .jitter_ms = 1.5,        // High jitter
        .sync_accuracy_ppm = 10.0,
        .buffer_underruns = 1,
        .buffer_overruns = 0
    };
    
    bool adapted = dynamic_profile_adaptation(&context, &poor_metrics);
    printf("Adaptation result: %s\n", adapted ? "Profile changed" : "No change");
    printf("Final profile: %s\n", context.current_profile->version_string);
    
    // Example 3: Profile compatibility matrix
    printf("\n3. Profile Compatibility Matrix\n");
    printf("================================\n");
    
    openavb_profile_cfg_t *milan_1_0 = openavb_profile_get_by_version(OPENAVB_SPEC_MILAN_1_0, OPENAVB_VARIANT_STRICT);
    openavb_profile_cfg_t *milan_1_1 = openavb_profile_get_by_version(OPENAVB_SPEC_MILAN_1_1, OPENAVB_VARIANT_STRICT);
    openavb_profile_cfg_t *avb_std = openavb_profile_get_by_version(OPENAVB_SPEC_IEEE_1722_2016, OPENAVB_VARIANT_STANDARD);
    
    if (milan_1_0 && milan_1_1 && avb_std) {
        printf("MILAN 1.0 <-> MILAN 1.1: %s\n", 
               openavb_profile_is_compatible(milan_1_0, milan_1_1) ? "Compatible" : "Incompatible");
        printf("MILAN 1.0 <-> AVB Std: %s\n", 
               openavb_profile_is_compatible(milan_1_0, avb_std) ? "Compatible" : "Incompatible");
        printf("MILAN 1.1 <-> AVB Std: %s\n", 
               openavb_profile_is_compatible(milan_1_1, avb_std) ? "Compatible" : "Incompatible");
    }
    
    printf("\n=== Demo Complete ===\n");
}

// Main function for testing
#ifdef PROFILE_DEMO_MAIN
int main(void)
{
    demonstrate_profile_abstraction();
    return 0;
}
#endif
