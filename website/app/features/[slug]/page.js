import FeatureArticle from "@/components/FeatureArticle";
import GlitcheLayout from "@/layouts/GlitcheLayout";
import { getAllFeatures, getFeatureBySlug } from "@/content/features/index";
import { serialize } from "next-mdx-remote/serialize";
import remarkGfm from "remark-gfm";
import { notFound } from "next/navigation";

export async function generateStaticParams() {
  const features = getAllFeatures();
  return features.filter((f) => f.hasDetail).map((f) => ({ slug: f.slug }));
}

const FeaturePage = async ({ params }) => {
  const feature = getFeatureBySlug(params.slug);
  if (!feature) notFound();

  const mdxSource = await serialize(feature.content, {
    mdxOptions: { remarkPlugins: [remarkGfm] },
  });

  return (
    <GlitcheLayout>
      <FeatureArticle
        title={feature.title}
        slug={params.slug}
        category={feature.category}
        mdxSource={mdxSource}
      />
    </GlitcheLayout>
  );
};

export default FeaturePage;