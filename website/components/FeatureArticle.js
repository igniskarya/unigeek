"use client";
import PageBanner from "@/components/PageBanner";
import { MDXRemote } from "next-mdx-remote";

const FeatureArticle = ({ title, category, slug, mdxSource }) => {
  return (
    <>
      <PageBanner
        pageName={title}
        pageLink={`/features/${slug}`}
        parentName="Features"
        parentLink="/features"
      />
      <div className="section blog">
        <div className="content">
          <div
            style={{
              textTransform: "uppercase",
              letterSpacing: "2px",
              fontSize: "0.7em",
              color: "#888",
              marginBottom: "24px",
            }}
          >
            {category}
          </div>
          <div className="single-post-text">
            <MDXRemote {...mdxSource} />
          </div>
          <div className="clear" />
        </div>
      </div>
    </>
  );
};

export default FeatureArticle;