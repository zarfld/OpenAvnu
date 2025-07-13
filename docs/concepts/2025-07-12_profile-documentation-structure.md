# Profile Documentation Structure and Consolidation Plan

**Background:**
OpenAvnu supports multiple AVB/TSN application profiles (e.g., MILAN, Automotive, Industrial, ProAV), each with different protocol requirements, supported features, and implementation details. Current documentation is fragmented, making it difficult for developers to find all relevant information for a given application profile or to compare profile versions and domain-specific requirements.

**Problem Statement:**
- Application profile-specific information (e.g., MILAN, Automotive, Industrial, ProAV) is scattered across multiple files and formats.
- Developers lack a single, comprehensive reference for each AVB/TSN application profile.
- Differences between application profiles and their protocol requirements are not clearly documented.

**Prior Decisions:**
- Hardware insight files are now exhaustive and consolidated.
- Documentation must be reference-quality, versioned, and highlight differences.

**Proposed Solution:**
- For each AVB/TSN application profile (MILAN, Automotive, Industrial, ProAV, etc.), create a dedicated documentation file in `docs/profiles/`.
- Each file will:
  - Consolidate all relevant technical, configuration, and implementation details for the application profile (including required protocols, supported features, and configuration).
  - Include version history and highlight differences between profile versions and between profiles.
  - Reference related hardware, protocol, test, and insight documentation.
  - Provide tables, diagrams, and code/configuration examples as needed.
- Example file naming: `docs/profiles/milan_profile_v1-v2.md`, `docs/profiles/automotive_profile.md`, etc.

**Alternatives Considered:**
- Continue with fragmented documentation (rejected: not developer-friendly).
- Use only high-level summaries (rejected: not sufficient for implementation).

**Risks and Open Questions:**
- Ensuring all relevant information is captured and kept up to date.
- How to best automate or validate cross-references between hardware, profile, and test docs?

**Next Steps:**
1. Create the `docs/profiles/` folder if it does not exist.
2. For each major AVB/TSN application profile (MILAN, Automotive, Industrial, ProAV, etc.):
   - Create a new documentation file.
   - Consolidate all relevant information (required protocols, specs, config, implementation, version differences, and domain-specific requirements).
   - Highlight version and cross-profile differences in dedicated sections or tables.
3. Update the documentation index to reference these new profile docs.
4. Review and iterate with the development team.
