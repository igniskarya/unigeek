import GlitcheLayout from "@/layouts/GlitcheLayout";
import PageBanner from "@/components/PageBanner";
import { getAllReleases } from "@/content/releases/index";
import { serialize } from "next-mdx-remote/serialize";
import remarkGfm from "remark-gfm";
import ReleaseList from "@/components/ReleaseList";

const ReleasesPage = async () => {
  const raw = getAllReleases();

  const releases = await Promise.all(
    raw.map(async ({ version, content }) => ({
      version,
      mdxSource: await serialize(content, {
        mdxOptions: { remarkPlugins: [remarkGfm] },
      }),
    }))
  );

  return (
    <GlitcheLayout>
      <PageBanner pageName="Release Notes" pageLink="/release-notes" />
      <ReleaseList releases={releases} />
    </GlitcheLayout>
  );
};

export default ReleasesPage;