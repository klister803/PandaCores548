#ifndef SHARED_PTRS_H
# define SHARED_PTRS_H

class Aura;
class UnitAura;
class AuraEffect;

# define AuraPtr std::shared_ptr<Aura>
# define UnitAuraPtr std::shared_ptr<UnitAura>
# define AuraEffectPtr std::shared_ptr<AuraEffect>
# define constAuraEffectPtr std::shared_ptr<const AuraEffect>

# define NULLAURA AuraPtr()
# define NULLAURA_EFFECT AuraEffectPtr()


#endif /* !SHARED_PTRS_H */