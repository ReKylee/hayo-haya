#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "hebrew/features.hpp"

namespace hyh::fairy {

    struct Entity;
    struct Program;

    enum class MentionContext {
        EntityReference,
        PropertyOwner,
        EventTarget,
        Speaker,
        CurrentActorPossessive
    };

    struct EntityMentionQuery {
        std::string surface;
        std::string headLemma;
        std::string roleLemma;

        bool definite = false;
        bool possessive = false;

        hyh::hebrew::Gender gender = hyh::hebrew::Gender::Unknown;
        hyh::hebrew::Number number = hyh::hebrew::Number::Unknown;

        std::size_t line = 1;
        MentionContext context = MentionContext::EntityReference;
        std::string currentActorName = {};
    };

    struct ScoredEntity {
        std::string entityName;
        int score = 0;
        std::vector<std::string> reasons;
    };

    struct ResolutionResult {
        enum class Kind {
            Resolved,
            Ambiguous,
            Unresolved
        };

        Kind kind = Kind::Unresolved;
        std::string entityName;
        std::vector<ScoredEntity> candidates;
    };

    class DiscourseResolver {
    public:
        explicit DiscourseResolver(const Program& program);

        [[nodiscard]] ResolutionResult resolveEntity(const EntityMentionQuery& query) const;

    private:
        [[nodiscard]] ScoredEntity scoreEntity(const Entity& entity, const EntityMentionQuery& query) const;
        [[nodiscard]] int recencyScore(std::size_t mentionLine, std::size_t entityLastMentionLine) const;

        const Program& program_;
    };

} // namespace hyh::fairy
