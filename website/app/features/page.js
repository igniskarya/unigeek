import FeatureGrid from "@/components/FeatureGrid";
import PageBanner from "@/components/PageBanner";
import GlitcheLayout from "@/layouts/GlitcheLayout";
import { getAllFeatures } from "@/content/features/index";

const FeaturesPage = () => {
  const features = getAllFeatures();
  return (
    <GlitcheLayout>
      <PageBanner pageName="Features" pageLink="/features" />
      <FeatureGrid features={features} />
    </GlitcheLayout>
  );
};

export default FeaturesPage;