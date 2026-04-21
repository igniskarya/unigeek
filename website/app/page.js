import Hero from '@/components/home/Hero';
import Specs from '@/components/home/Specs';
import Categories from '@/components/home/Categories';
import BoardSummary from '@/components/home/BoardSummary';
import InstallCta from '@/components/home/InstallCta';
import { BOARDS } from '@/content/boards';
import { CATALOG, CATEGORIES, countsByCategory } from '@/content/features/catalog';

export default function HomePage() {
  const moduleCount = CATALOG.length;
  const boardCount = BOARDS.length;
  const counts = countsByCategory();

  return (
    <>
      <Hero moduleCount={moduleCount} boardCount={boardCount} />
      <Specs moduleCount={moduleCount} boardCount={boardCount} />
      <Categories categories={CATEGORIES} counts={counts} total={moduleCount} />
      <BoardSummary boards={BOARDS} />
      <InstallCta />
    </>
  );
}
