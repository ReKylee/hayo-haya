#include "fairy/discourse_resolver.hpp"

#include <algorithm>

#include "fairy/story_ast.hpp"

namespace hyh::fairy {
    namespace {

        bool compatibleGender(hyh::hebrew::Gender entityGender, hyh::hebrew::Gender queryGender) {
            return entityGender == hyh::hebrew::Gender::Unknown || queryGender == hyh::hebrew::Gender::Unknown
                || entityGender == queryGender || entityGender == hyh::hebrew::Gender::Common
                || queryGender == hyh::hebrew::Gender::Common;
        }

        bool compatibleNumber(hyh::hebrew::Number entityNumber, hyh::hebrew::Number queryNumber) {
            return entityNumber == hyh::hebrew::Number::Unknown || queryNumber == hyh::hebrew::Number::Unknown
                || entityNumber == queryNumber;
        }

        bool contains(const std::vector<std::string>& values, const std::string& value) {
            return std::ranges::find(values, value) != values.end();
        }

    } // namespace

    DiscourseResolver::DiscourseResolver(const Program& program) : program_(program) {}

    int DiscourseResolver::recencyScore(std::size_t mentionLine, std::size_t entityLastMentionLine) const {
        if (entityLastMentionLine == 0 || entityLastMentionLine > mentionLine) {
            return 0;
        }

        const auto distance = mentionLine - entityLastMentionLine;
        if (distance == 0)
            return 30;
        if (distance == 1)
            return 25;
        if (distance <= 3)
            return 18;
        if (distance <= 6)
            return 10;
        if (distance <= 12)
            return 4;
        return 0;
    }

    ScoredEntity DiscourseResolver::scoreEntity(const Entity& entity, const EntityMentionQuery& query) const {
        ScoredEntity scored;
        scored.entityName = entity.name;

        if (!compatibleGender(entity.gender, query.gender)) {
            scored.score = -1000;
            scored.reasons.push_back("gender mismatch");
            return scored;
        }
        if (!compatibleNumber(entity.number, query.number)) {
            scored.score = -1000;
            scored.reasons.push_back("number mismatch");
            return scored;
        }

        if (!query.surface.empty() && entity.name == query.surface) {
            scored.score += 100;
            scored.reasons.push_back("exact surface/name match");
        }
        if (!query.headLemma.empty() && entity.headLemma == query.headLemma) {
            scored.score += 60;
            scored.reasons.push_back("head lemma match");
        }
        if (!query.roleLemma.empty() && contains(entity.roles, query.roleLemma)) {
            scored.score += 45;
            scored.reasons.push_back("role lemma match");
        }
        if (query.definite) {
            scored.score += 20;
            scored.reasons.push_back("definite mention prefers known entity");
        }
        if (!query.currentActorName.empty() && entity.name == query.currentActorName) {
            scored.score += 50;
            scored.reasons.push_back("current actor match");
        }
        if (entity.gender != hyh::hebrew::Gender::Unknown && query.gender != hyh::hebrew::Gender::Unknown) {
            scored.score += 15;
            scored.reasons.push_back("gender compatible");
        }
        if (entity.number != hyh::hebrew::Number::Unknown && query.number != hyh::hebrew::Number::Unknown) {
            scored.score += 15;
            scored.reasons.push_back("number compatible");
        }

        scored.score += recencyScore(query.line, entity.lastMentionLine);
        if (entity.mentionCount > 0) {
            scored.score += static_cast<int>(std::min<std::size_t>(entity.mentionCount, 5) * 3);
        }

        return scored;
    }

    ResolutionResult DiscourseResolver::resolveEntity(const EntityMentionQuery& query) const {
        std::vector<ScoredEntity> scored;
        for (const auto& [_, entity]: program_.entities) {
            auto candidate = scoreEntity(entity, query);
            if (candidate.score > -1000) {
                scored.push_back(std::move(candidate));
            }
        }

        std::ranges::sort(scored, [](const auto& left, const auto& right) {
            return left.score > right.score;
        });

        ResolutionResult result;
        result.candidates = std::move(scored);
        if (result.candidates.empty() || result.candidates.front().score < 50) {
            result.kind = ResolutionResult::Kind::Unresolved;
            return result;
        }

        if (result.candidates.size() > 1 && result.candidates.front().score - result.candidates[1].score < 20) {
            result.kind = ResolutionResult::Kind::Ambiguous;
            return result;
        }

        result.kind = ResolutionResult::Kind::Resolved;
        result.entityName = result.candidates.front().entityName;
        return result;
    }

} // namespace hyh::fairy
