# Section X: [Section Name]

**Integration Status**: ⏳ Pending / 🔄 In Progress / ✅ Implemented
**Implementation Location**: [Subsystem/Class names]
**Commit**: [Commit hash if implemented]
**Breaking Changes**: None / [Description if any]

---

## Overview

[Brief description of what this section covers]

---

## X-1. [First Topic]

[Rule text from official comprehensive rules]

### Rule X-1-1. [Sub-rule]
[Detailed rule text]

**Implementation**:
- Location: [File path and class/function name]
- Method: [Method name or approach]
- Validation: [How this rule is enforced]
- Field/Enum: [Related data structures if any]

```cpp
// Example code showing implementation
// Use rule references in comments: // Rule X-1-1: Description
```

**Testing Notes**:
- [Test case 1]
- [Test case 2]
- [Edge cases to consider]

---

## X-2. [Second Topic]

[Continue same pattern for all rules in this section]

---

## Implementation Checklist

- [ ] **X-1**: [Rule description]
- [ ] **X-1-1**: [Sub-rule description]
- [ ] **X-2**: [Rule description]
- [ ] **X-2-1**: [Sub-rule description]

---

## Modular Integration Notes

**For implementers:**

1. **Add validation to ComprehensiveRulesSubsystem**:
   - Create new method: `ValidateRule_X_Y()`
   - Return validation result with rule reference
   - Call from appropriate subsystem

2. **Avoid modifying core types**:
   - Do NOT change enums in GCGTypes.h
   - Do NOT add fields to FGCGCardData unless absolutely necessary
   - Use external validation methods instead

3. **Reference rules by number**:
   ```cpp
   // Rule X-2-3: Example rule description
   if (!ValidateCondition())
   {
       return FGCGResult(false, "Violates Rule X-2-3: Example rule");
   }
   ```

4. **Document, don't duplicate**:
   - Keep official rules text in this markdown file
   - Reference this file from code comments
   - Don't copy-paste rules into code

---

## Related Sections

- [Link to related section 1]
- [Link to related section 2]

---

## Official Rules Reference

All rules text is from the official Gundam Trading Card Game Comprehensive Rules, Section X.
